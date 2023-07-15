#pragma once

#include "nekoav.hpp"
#include "nekowrap.hpp"

#include <QThread>

#include <condition_variable>
#include <atomic>
#include <mutex>
#include <deque>


namespace NekoAV {

inline static auto EofPacket = nullptr;
inline static auto FlushPacket = reinterpret_cast<AVPacket*>(0x01);
inline static auto SyncPacket = reinterpret_cast<AVPacket*>(0x03);
inline static auto AVSyncThreshold = 0.01;
inline static auto AVNoSyncThreshold = 10.0;
inline static auto AudioDiffAvgNB = 20;

template <typename T>
using Atomic = std::atomic<T>;

using namespace std::chrono_literals;

class PacketQueue final {
    public:
        PacketQueue();
        PacketQueue(const PacketQueue &) = delete;
        ~PacketQueue();

        void flush();
        void put(AVPacket *packet);
        void unget(AVPacket *packet);
        bool seek(int64_t pos);
        void requestStop();
        auto get(bool blocking = true) -> AVPacket *;
        size_t size() const;
        int64_t duration() const;
        bool    stopRequested() const;
    private:
        std::deque<AVPacket*>   packets;
        std::condition_variable cond;
        std::mutex              condMutex;
        Atomic<int64_t>         packetsDuration = 0; //< Sums of packet duration
        Atomic<bool>            stop = false;
        mutable std::mutex      mutex;
};

class DemuxerThread;

class AudioThread final : public QObject {
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
        void audioCallback(void *data, int datasize);
        int  audioDecodeFrame();
        int  audioResample(int outSamples);
        void run();

        DemuxerThread  *demuxerThread = nullptr;
        AVCodecContext *codecCtxt = nullptr;
        AVStream       *stream = nullptr;

        AudioOutput    *audioOutput = nullptr;
        QThread        *decoderThread = nullptr;

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

        AudioSampleFormat    outputSampleFormat{ };
        int                  outputSampleRate{ };
        int                  outputChannels{ };

        // Status
        Atomic<bool>   waitting = false;
        Atomic<qreal>  audioClock = 0.0f;
};

class VideoThread final : public QObject {
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
        void run();

        DemuxerThread  *demuxerThread = nullptr;
        AVCodecContext *codecCtxt = nullptr;
        AVStream       *stream = nullptr;

        VideoSink      *videoSink = nullptr;
        PacketQueue     queue;

        // Thread
        QThread        *presentThread = nullptr; //< for Write frames

        // Frame
        AVPtr<SwsContext> swsCtxt;
        AVPtr<AVFrame> srcFrame {av_frame_alloc()};
        AVPtr<AVFrame> dstFrame {av_frame_alloc()};
        std::mutex     dstFrameMutex;

        // Hardware
        AVPtr<AVFrame> swFrame {av_frame_alloc()};
        AVPixelFormat hardwarePixfmt = AV_PIX_FMT_NONE;

        // Sync 
        std::condition_variable cond;
        std::mutex   condMutex;

        // Status
        int64_t videoClockStart = 0; //< Video started time
        double  swsScaleDuration = 0.0; //< prev Swscale take's time
        double  videoDecodeDuration = 0.0; //< prev video decode duration
        bool    firstFrame = true; //< first frame arrives
        bool    needConvert = true;

        // Atomoic Status 
        Atomic<bool>   paused = false;
        Atomic<bool>   waitting = false;
        Atomic<double> videoClock = 0.0f;
        Atomic<uint64_t> videoFrameCount = 0; //< All frames received count
        Atomic<uint64_t> videoDropedFrameCount = 0; //< Droped frame count
};

class SubtitleThread final : public QThread {
    Q_OBJECT
    public:
        SubtitleThread(DemuxerThread *parent, AVStream *stream, AVCodecContext *ctxt);
        ~SubtitleThread();

        bool idle() const {
            // return waitting;
            return queue.size() == 0;
        }
        bool isPaused() const {
            return paused;
        }
        PacketQueue &packetQueue() noexcept {
            return queue;
        }
        void pause(bool v);
        void switchStream(AVStream *stream);
        void refresh();
    private:
        void run() override;

        DemuxerThread  *demuxerThread = nullptr;
        AVCodecContext *codecCtxt = nullptr;
        AVStream       *stream = nullptr;

        VideoSink      *videoSink = nullptr;
        PacketQueue     queue;        

        // Sync 
        std::condition_variable cond;
        std::mutex   condMutex;
        Atomic<bool> paused = false;
        Atomic<bool> waitting = false;
        Atomic<bool> reqRefresh = false;

        Atomic<double> subtitleClock = 0.0;
};

class DemuxerThread final : public QThread {
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
        void requestSwitchStream(int stream);
        void doPause(bool v);

        /**
         * @brief Current position
         * 
         * @return qreal 
         */
        qreal clock() const;
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
        int  interruptHandler();

        AVIOContext     *ioCtxt = nullptr; //< Custom IO Context
        AVFormatContext *formatCtxt = nullptr; //< Container of format context
        AVPacket        *packet = nullptr; //< Allocated packet

        MediaPlayerPrivate *player = nullptr; //< Player of the manager

        AudioThread        *audioThread = nullptr;
        VideoThread        *videoThread = nullptr;
        SubtitleThread     *subtitleThread = nullptr;

        QObject            *invokeHelper = nullptr;

        int                 errcode = 0;
        bool                quit = false;
        bool                hasSeek = false;
        bool                isReading = false;
        bool                wakeupOnce = false; //< When call wakeup, set it to true, and clear in InterruptHandler
        bool                afterSeek = false;
        qreal               seekPosition = 0;
        qreal               curPosition = 0;

        int64_t             externalClockStart = 0;
        qreal               externalClock = 0.0; //< External clock

        uint8_t            *ioBuffer = nullptr;
        int                 ioBufferSize = 0;

        // Buffering     
        int                 bufferedPacketsLimit = 4000;
        int                 bufferedPacketsLessThreshold = 50;
        int                 bufferedPacketsEnough = 100;
        float               prevBufferProgress = 0.0f;
        int64_t             prevTooLessPacketsTime = 0; //< Previous buffer data not enough time

        // Stream info
        bool                isLocalSource = false; //< If source is local, no need to buffering

        std::condition_variable cond;
        std::mutex              condMutex;
};

class MediaPlayerPrivate final : public QObject {
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
        QUrl          url; //< Player Url
        QIODevice    *ioDevice; //< IODevice for playback
        AVDictionary *options = nullptr;
        AVInputFormat *inputFormat = nullptr; //< User custom input format

        bool          loaded = false;
        int           audioStream = -1;
        int           videoStream = -1;
        int           subtitleStream = -1;
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
    friend class MediaPlayer;
};

class VideoFramePrivate final {
    public:
        explicit VideoFramePrivate(AVFrame *f) : frame(f) { }
        VideoFramePrivate(const VideoFramePrivate &) = delete;
        ~VideoFramePrivate() {
            if (frame) {
                av_frame_unref(frame.get());
            }
        }
        AVPtr<AVFrame> frame;
};

inline AudioOutput *DemuxerThread::audioOutput() const noexcept {
    return player->audioOutput;
}
inline VideoSink   *DemuxerThread::videoSink() const  noexcept {
    return player->videoSink;
}

inline bool    IsSpecialPacket(AVPacket *pak) noexcept {
    return pak == EofPacket || pak == FlushPacket || pak == SyncPacket;
}
inline AVPixelFormat ToAVPixelFormat(VideoPixelFormat fmt) {
    switch (fmt) {
        case VideoPixelFormat::RGBA32 : return AV_PIX_FMT_RGBA;
        case VideoPixelFormat::RGB24  : return AV_PIX_FMT_RGB24;
        case VideoPixelFormat::YUV420P : return AV_PIX_FMT_YUV420P;
        case VideoPixelFormat::YUYV422 : return AV_PIX_FMT_YUYV422;
        case VideoPixelFormat::UYVY422 : return AV_PIX_FMT_UYVY422;
        case VideoPixelFormat::NV12 : return AV_PIX_FMT_NV12;
        case VideoPixelFormat::NV21 : return AV_PIX_FMT_NV21;
        case VideoPixelFormat::Invalid :
        default : return AV_PIX_FMT_NONE;
    }
}
inline VideoPixelFormat ToVideoPixelFormat(AVPixelFormat fmt) {
    switch (fmt) {
        case AV_PIX_FMT_RGB24 : return VideoPixelFormat::RGB24;
        case AV_PIX_FMT_RGBA : return VideoPixelFormat::RGBA32;
        case AV_PIX_FMT_YUV420P : return VideoPixelFormat::YUV420P;
        case AV_PIX_FMT_YUYV422 : return VideoPixelFormat::YUYV422;
        case AV_PIX_FMT_UYVY422 : return VideoPixelFormat::UYVY422;
        case AV_PIX_FMT_NV12 : return VideoPixelFormat::NV12;
        case AV_PIX_FMT_NV21 : return VideoPixelFormat::NV21;
        default :              return VideoPixelFormat::Invalid;
    }
}

}