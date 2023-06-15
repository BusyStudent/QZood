#pragma once

#include "nekoav.hpp"

#include <QThread>

#include <condition_variable>
#include <atomic>
#include <mutex>

#include <queue>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/time.h>
    #include <libavutil/avutil.h>
    #include <libavutil/imgutils.h>
    #include <libswresample/swresample.h>
    #include <libswscale/swscale.h>
}

namespace NekoAV {

inline static auto EofPacket = nullptr;
inline static auto FlushPacket = reinterpret_cast<AVPacket*>(0x01);
inline static auto StopPacket = reinterpret_cast<AVPacket*>(0x02);
inline static auto AVSyncThreshold = 0.01;
inline static auto AVNoSyncThreshold = 1.0;
inline static auto AudioDiffAvgNB = 20;

using namespace std::chrono_literals;

template <typename T>
class AVTraits;

template <>
class AVTraits<AVPacket> {
    public:
        void operator()(AVPacket *ptr) {
            av_packet_free(&ptr);
        }
};

template <>
class AVTraits<AVFrame> {
    public:
        void operator()(AVFrame *ptr) {
            av_frame_free(&ptr);
        }
};

template <>
class AVTraits<AVFormatContext> {
    public:
        void operator()(AVFormatContext *ptr) {
            avformat_close_input(&ptr);
        }
};

template <>
class AVTraits<AVCodecContext> {
    public:
        void operator()(AVCodecContext *ptr) {
            avcodec_free_context(&ptr);
        }
};

template <>
class AVTraits<AVDictionary> {
    public:
        void operator()(AVDictionary *ptr) {
            av_dict_free(&ptr);
        }
};

template <>
class AVTraits<SwrContext> {
    public:
        void operator()(SwrContext *ptr) {
            swr_free(&ptr);
        }
};

template <>
class AVTraits<SwsContext> {
    public:
        void operator()(SwsContext *ptr) {
            sws_freeContext(ptr);
        }
};

template <>
class AVTraits<uint8_t> {
    public:
        void operator()(uint8_t *ptr) {
            av_free(ptr);
        }
};

template <typename T>
class AVPtr : public std::unique_ptr<T, AVTraits<T>> {
    public:
        using std::unique_ptr<T, AVTraits<T>>::unique_ptr;
};

class PacketQueue {
    public:
        PacketQueue();
        PacketQueue(const PacketQueue &) = delete;
        ~PacketQueue();

        void flush();
        void put(AVPacket *packet);
        auto get(bool blocking = true) -> AVPacket *;
        size_t size() const;
        int64_t duration() const;
    private:
        std::queue<AVPacket*> packets;
        std::condition_variable cond;
        std::mutex              condMutex;
        int64_t                 packetsDuration = 0; //< Sums of packet duration
        mutable std::mutex      mutex;
};

class DemuxerThread;

class AudioThread : public QObject {
    Q_OBJECT
    public:
        AudioThread(DemuxerThread *parent, AVStream *stream, AVCodecContext *ctxt);
        ~AudioThread();
        
        bool idle() const {
            // return waitting;
            return queue.size() == 0;
        }
        bool isOk() const {
            return audioInitialized;
        }
        bool isPaused() const {
            return audioOutput->isPaused();
        }
        qreal clock() const {
            return audioClock;
        }
        
        PacketQueue &packetQueue() noexcept {
            return queue;
        }
        void pause(bool v);
    private:
        void audioOutputLost();
        void audioCallback(void *data, int datasize);
        int  audioDecodeFrame();
        int  audioResample(int outSamples);

        DemuxerThread  *demuxerThread = nullptr;
        AVCodecContext *codecCtxt = nullptr;
        AVStream       *stream = nullptr;

        AudioOutput    *audioOutput = nullptr;

        PacketQueue     queue;

        // Frame decode to
        AVPtr<AVFrame>       frame {av_frame_alloc()};
        AVPtr<SwrContext>    swrCtxt{ };
        AVPtr<uint8_t>       swrBuffer{ }; //< Buffers of resampled data
        uint8_t             *buffer = nullptr; //< Buffer pointer to address of readable
        int                  bufferIndex = 0; //< Position in buffer, (in byte)
        int                  bufferSize = 0; //< Size of buffer
        bool                 needResample = false;
        bool                 audioInitialized = false;

        // Status
        bool   waitting = false;
        qreal  audioClock = 0.0f;
};

class VideoThread : public QThread {
    Q_OBJECT
    public:
        VideoThread(DemuxerThread *parent, AVStream *stream, AVCodecContext *ctxt);
        ~VideoThread();

        bool idle() const {
            return waitting;
        }
        bool isPaused() const {
            return paused;
        }
        qreal clock() const {
            return videoClock;
        }

        PacketQueue &packetQueue() noexcept {
            return queue;
        }
        void pause(bool v);
    private:
        bool videoDecodeFrame(AVPacket *packet, AVFrame **ret);
        void videoWriteFrame(AVFrame *source);
        void tryHardwareInit();
        void run() override;

        DemuxerThread  *demuxerThread = nullptr;
        AVCodecContext *codecCtxt = nullptr;
        AVStream       *stream = nullptr;

        VideoSink      *videoSink = nullptr;
        PacketQueue     queue;

        // Frame
        AVPtr<SwsContext> swsCtxt;
        AVPtr<AVFrame> srcFrame {av_frame_alloc()};
        AVPtr<AVFrame> dstFrame {av_frame_alloc()};
        std::mutex     dstFrameMutex;

        // Hardware
        AVPtr<AVFrame> swFrame {av_frame_alloc()};
        AVPixelFormat hardwarePixfmt = AV_PIX_FMT_NONE;

        // Sync 
        std::thread thrd;
        std::condition_variable cond;
        std::mutex condMutex;
        bool paused = false;

        // Status
        int64_t videoClockStart = 0; //< Video started time
        double  videoClock = 0.0f;
        double  swsScaleDuration = 0.0; //< prev Swscale take's time
        double  videoDecodeDuration = 0.0; //< prev video decode duration
        bool    waitting = false;

};

class DemuxerThread : public QThread {
    Q_OBJECT
    public:
        using MediaStatus = MediaPlayer::MediaStatus;
        using PlaybackState = MediaPlayer::PlaybackState;
        using Loops = MediaPlayer::Loops;

        DemuxerThread(MediaPlayerPrivate *parent);
        ~DemuxerThread();

        void run() override;
        /**
         * @brief Wake the demuxer if it is waiting
         * 
         */
        void wakeUp();
        void requestSeek(qreal position);
        void doPause(bool v);

        /**
         * @brief Current position
         * 
         * @return qreal 
         */
        qreal position() const;
        qreal bufferedDuration() const;
        float bufferProgress() const;

        AVFormatContext *formatContext() const noexcept {
            return formatCtxt;
        }
        AudioOutput     *audioOutput() const noexcept;
        VideoSink       *videoSink() const  noexcept;
    Q_SIGNALS:
        void ffmpegBuffering(qreal duration, float progress);
        void ffmpegMediaStatusChanged(MediaStatus status);
        void ffmpegPlaybackStateChanged(PlaybackState state);
        void ffmpegPositionChanged(qreal pos);
        void ffmpegErrorOccurred(int avcode);
        void ffmpegMediaLoaded();
    private:
        bool load();
        bool prepareWorker();
        bool prepareCodec(int stream);
        bool sendError(int avcode);
        bool runDemuxer();
        bool readFrame(int *eof);
        bool tooMuchPackets() const;
        bool tooLessPackets() const;
        bool hasEnoughPackets() const;
        bool waitForEvent(std::chrono::milliseconds ms);
        bool isPictureStream(int idx) const;
        void doUpdateClock();
        bool doSeek();

        AVIOContext     *ioCtxt = nullptr; //< Custom IO Context
        AVFormatContext *formatCtxt = nullptr; //< Container of format context
        AVPacket        *packet = nullptr; //< Allocated packet

        MediaPlayerPrivate *player = nullptr; //< Player of the manager

        AudioThread        *audioThread = nullptr;
        VideoThread        *videoThread = nullptr;

        int                 errcode = 0;
        bool                quit = false;
        bool                hasSeek = false;
        qreal               seekPosition = 0;
        qreal               curPosition = 0;

        int64_t             externalClockStart = 0;
        qreal               externalClock = 0.0; //< External clock

        uint8_t            *ioBuffer = nullptr;
        int                 ioBufferSize = 0;

        // Buffering     
        int                 bufferedPacketsLimit = 4000;
        int                 bufferedPacketsLessThreshold = 100;
        int                 bufferedPacketsEnough = 500;
        float               prevBufferProgress = 0.0f;
        int64_t             prevTooLessPacketsTime = 0; //< Previous buffer data not enough time

        // Stream info
        bool                isLocalSource = false; //< If source is local, no need to buffering

        std::condition_variable cond;
        std::mutex              condMutex;
};

class MediaPlayerPrivate : public QObject {
    Q_OBJECT
    public:
        using MediaStatus = MediaPlayer::MediaStatus;
        using PlaybackState = MediaPlayer::PlaybackState;
        using Loops = MediaPlayer::Loops;
        using Error = MediaPlayer::Error;

        MediaPlayerPrivate(MediaPlayer *player) : player(player) { }
        ~MediaPlayerPrivate();

        AVFormatContext *formatContext() const noexcept {
            if (!demuxerThread) {
                return nullptr;
            }
            return demuxerThread->formatContext();
        }

        void load();
        void play();
        void stop();
        void pause();
        void setPlaybackState(PlaybackState s);
        void setMediaStatus(MediaStatus s);


        std::mutex    settingsMutex; //< Mutex for protect this

        // Begin settingsMutex protect
        QString       url; //< Player Url
        QIODevice    *ioDevice; //< IODevice for playback
        AVDictionary *options = nullptr;
        AVInputFormat *inputFormat = nullptr; //< User custom input format

        bool          loaded = false;
        int           audioStream = -1;
        int           videoStream = -1;
        int           loops = Loops::Once;

        // End 
        AudioOutput  *audioOutput = nullptr;
        VideoSink    *videoSink = nullptr;

        MediaPlayer  *player = nullptr;
        DemuxerThread *demuxerThread = nullptr;

        MediaStatus   mediaStatus = MediaStatus::NoMedia;
        PlaybackState playbackState = PlaybackState::StoppedState;

        Error         error = Error::NoError;
        QString       errorString;
    private:
        void demuxerBuffering(qreal duration, float progress);
        void demuxerErrorOccurred(int errcode);
        void demuxerPositionChanged(qreal pos);
        void demuxerMediaLoaded();
        void updateMediaInfo();
        void audioDeviceLost();
    friend class MediaPlayer;
};

class VideoFramePrivate : public AVFrame {

};

inline AudioOutput *DemuxerThread::audioOutput() const noexcept {
    return player->audioOutput;
}
inline VideoSink   *DemuxerThread::videoSink() const  noexcept {
    return player->videoSink;
}

// Helper function
inline int64_t doubelToFFInt64(qreal d) noexcept {
    return d * 1000000.0;
}
inline qreal   FFInt64ToDouble(int64_t t) noexcept {
    return t / 1000000.0;
}
inline bool    IsSpecialPacket(AVPacket *pak) noexcept {
    return pak == EofPacket || pak == FlushPacket || pak == StopPacket;
}
inline QString FFErrorToString(int errcode) {
    char buffer[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(buffer, sizeof(buffer), errcode);
}
inline AVPtr<uint8_t>  FFAllocateBuffer(size_t n) {
    return AVPtr<uint8_t>(static_cast<uint8_t*>(av_malloc(n)));
}
inline AVPtr<uint8_t> &FFReallocateBuffer(AVPtr<uint8_t> *old, size_t newSize) {
    old->reset(
        static_cast<uint8_t*>(av_realloc(
            old->release(),
            newSize
        ))
    );
    return *old;
}

}