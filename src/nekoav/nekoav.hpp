#pragma once

#include <QGraphicsItem>
#include <QWidget>
#include <QString>
#include <QObject>
#include <QUrl>

#if   defined(_MSC_VER)
    #define NEKO_EXPORT 	__declspec(dllexport)
    #define NEKO_IMPORT     __declspec(dllimport)
#elif defined(__GNUC__) && defined(_WIN32)
    #define NEKO_EXPORT 	__attribute__(dllexport)
    #define NEKO_IMPORT     __attribute__(dllimport)
#else
    #define NEKO_EXPORT
    #define NEKO_IMPORT
#endif

#if   defined(NEKO_SOURCE)
    #define NEKO_API       NEKO_EXPORT
#else
    #define NEKO_API       NEKO_IMPORT
#endif

#define NEKO_USING(n) using Neko##n = NekoAV::n
#define NEKO_API

namespace NekoAV {

class GraphicsVideoItemPrivate;
class VideoWidgetPrivate;
class VideoFramePrivate;
class MediaPlayerPrivate;
class AudioOutputPrivate;
class VideoSink;

// Enums
enum class AudioSampleFormat {
    Uint8,
    Sint16,
    Sint32,
    Float32,
};
enum class VideoPixelFormat {
    Invalid,
    RGBA32,
};


class NEKO_API VideoFrame {
    public:
        VideoFrame() = default;
        VideoFrame(const VideoFrame &) = default;
        ~VideoFrame() = default;

        int width() const;
        int height() const;
        QSize size() const;
        bool  isNull() const;
        int   planeCount() const;

        uchar *bits(int plane) const;
        int    bytesPerLine(int plane) const;
        VideoPixelFormat pixelFormat() const;

        void   lock() const;
        void   unlock() const;

        static VideoFrame fromAVFrame(void *avf);
    private:
        VideoFramePrivate *d = nullptr;
};

class NEKO_API GraphicsVideoItem : public QGraphicsObject {
    Q_OBJECT
    public:
        explicit GraphicsVideoItem(QGraphicsObject *parent = nullptr);
        ~GraphicsVideoItem();

        /**
         * @brief Get video sink of this widget
         * 
         * @return VideoSink* 
         */
        VideoSink *videoSink() const;
        QRectF boundingRect() const;
        QSizeF nativeSize() const;
        void   setSize(const QSizeF &);

        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    Q_SIGNALS:
        void nativeSizeChanged(const QSizeF &size);
    private:
        QScopedPointer<GraphicsVideoItemPrivate> d;
};
class NEKO_API VideoWidget       : public QWidget {
    Q_OBJECT
    public:
        explicit VideoWidget(QWidget *parent = nullptr);
        ~VideoWidget();

        /**
         * @brief Get video sink of this widget
         * 
         * @return VideoSink* 
         */
        VideoSink *videoSink() const;
    protected:
        void paintEvent(QPaintEvent *) override;
    private:
        QScopedPointer<VideoWidgetPrivate> d;
};

class NEKO_API VideoSink  : public QObject {
    Q_OBJECT
    public:
        explicit VideoSink(QObject *parent = nullptr);
        ~VideoSink();

        void putVideoFrame(const VideoFrame &frame);
        QSize videoSize() const;
    Q_SIGNALS:
        void videoFrameChanged(const VideoFrame &frame);
        void videoSizeChanged();
    private:
        VideoFrame frame;
        QSize      size = {0, 0};
};

/**
 * @brief Audio Output 
 * 
 */
class NEKO_API AudioOutput : public QObject {
    Q_OBJECT
    public:
        using Routinue = std::function<void(void *buffer, int bufferSize)>;

        explicit AudioOutput(QObject *parent = nullptr);
        AudioOutput(const AudioOutput &) = delete;
        ~AudioOutput();

        bool open(AudioSampleFormat format, int sampleRate, int channels);
        void pause(bool on);
        bool close();
        void setVolume(float volume);
        void setCallback(const Routinue &);
        void setMuted(bool v);

        bool isOpen() const;
        bool isPaused() const;
        bool isMuted() const;
        float volume() const;
    Q_SIGNALS:
        void volumeChanged(float volume);
        void mutedChanged(bool muted);
    private:
        QScopedPointer<AudioOutputPrivate> d;
};

class NEKO_API MediaPlayer : public QObject {
    Q_OBJECT
    public:
        enum PlaybackState {
            StoppedState,
            PlayingState,
            PausedState
        };
        Q_ENUM(PlaybackState)

        enum MediaStatus {
            NoMedia,
            LoadingMedia,
            LoadedMedia,
            StalledMedia,
            BufferingMedia,
            BufferedMedia,
            EndOfMedia,
            InvalidMedia
        };
        Q_ENUM(MediaStatus)

        enum Error {
            NoError,
            ResourceError,
            FormatError,
            NetworkError,
            AccessDeniedError,
            UnknownError
        };
        Q_ENUM(Error)

        enum Loops {
            Infinite = -1,
            Once = 1
        };
        Q_ENUM(Loops)

        explicit MediaPlayer(QObject *parent = nullptr);
        ~MediaPlayer();

        // QList<QMediaMetaData> audioTracks() const;
        // QList<QMediaMetaData> videoTracks() const;
        // QList<QMediaMetaData> subtitleTracks() const;

        int activeAudioTrack() const;
        int activeVideoTrack() const;
        int activeSubtitleTrack() const;

        void setActiveAudioTrack(int index);
        void setActiveVideoTrack(int index);
        void setActiveSubtitleTrack(int index);

        void setAudioOutput(AudioOutput *output);
        AudioOutput *audioOutput() const;

        void setVideoOutput(GraphicsVideoItem *);
        void setVideoOutput(VideoWidget *);
        // QObject *videoOutput() const;

        void setVideoSink(VideoSink *sink);
        // QVideoSink *videoSink() const;

        QUrl source() const;
        // const QIODevice *sourceDevice() const;

        PlaybackState playbackState() const;
        MediaStatus mediaStatus() const;

        qreal duration() const;
        qreal position() const;

        bool hasAudio() const;
        bool hasVideo() const;

        float bufferProgress() const;
        // QMediaTimeRange bufferedTimeRange() const;

        bool isSeekable() const;
        qreal playbackRate() const;

        bool isPlaying() const;
        bool isLoaded() const;

        int loops() const;
        void setLoops(int loops);

        Error error() const;
        QString errorString() const;

        bool isAvailable() const;
    public Q_SLOTS:
        void play();
        void pause();
        void stop();
        void load();

        void setPosition(qreal position);

        void setPlaybackRate(qreal rate);

        void setSource(const QUrl &source);
        void setSourceDevice(QIODevice *device, const QUrl &sourceUrl = QUrl());
        void setOption(const QString &key, const QString &value);
        void setHttpUseragent(const QString &useragent);
        void setHttpReferer(const QString &referer);
        void clearOptions();
    Q_SIGNALS:
        void sourceChanged(const QUrl &media);
        void playbackStateChanged(PlaybackState newState);
        void mediaStatusChanged(MediaStatus status);

        void durationChanged(qreal duration);
        void positionChanged(qreal position);

        void hasAudioChanged(bool available);
        void hasVideoChanged(bool videoAvailable);

        void bufferProgressChanged(float progress);

        void seekableChanged(bool seekable);
        void playingChanged(bool playing);
        void playbackRateChanged(qreal rate);
        void loopsChanged();

        void metaDataChanged();
        void videoOutputChanged();
        void audioOutputChanged();

        void tracksChanged();
        void activeTracksChanged();

        void errorChanged();
        void errorOccurred(MediaPlayer::Error error, const QString &errorString);

    private:
        QScopedPointer<MediaPlayerPrivate> d;
};

}

// Export namespace
NEKO_USING(GraphicsVideoItem);
NEKO_USING(AudioSampleFormat);
NEKO_USING(VideoPixelFormat);
NEKO_USING(VideoFrame);
NEKO_USING(VideoSink);
NEKO_USING(VideoWidget);
NEKO_USING(MediaPlayer);
NEKO_USING(AudioOutput);

