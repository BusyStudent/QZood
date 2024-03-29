#pragma once

#include <QGraphicsItem>
#include <QWidget>
#include <QString>
#include <QObject>
#include <QUrl>

#if   defined(_MSC_VER) && defined(NEKO_DLL)
    #define NEKO_EXPORT 	__declspec(dllexport)
    #define NEKO_IMPORT     __declspec(dllimport)
#elif defined(__GNUC__) && defined(NEKO_DLL) && defined(_WIN32)
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

struct AVFrame;
struct AVDictionary;
struct AVInputFormat;

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

    // RGB formats
    RGBA32,
    RGB24,

    //< YUV format
    IYUV,
    YUY2,
    UYVY,

    NV12,
    NV21,

    //< YUV format alias
    YUV420P = IYUV,
    YUYV422 = YUY2,
    UYVY422 = UYVY,
};


class NEKO_API VideoFrame {
    public:
        VideoFrame();
        VideoFrame(const VideoFrame &);
        VideoFrame(VideoFrame &&);
        ~VideoFrame();

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

        QImage toImage() const;

        VideoFrame &operator =(const VideoFrame &);
        VideoFrame &operator =(VideoFrame &&);
        
        static VideoFrame fromAVFrame(AVFrame *avf);
    private:
        QSharedPointer<VideoFramePrivate> d;
};

class NEKO_API Dictionary : public QMap<QString, QString> {
    public:
        using QMap<QString, QString>::QMap;

        void          load(const AVDictionary *dict);
        AVDictionary *toAVDictionary() const;
        
        static Dictionary fromAVDictionary(const AVDictionary *dict) {
            Dictionary d;
            d.load(dict);
            return d;
        }
};

class NEKO_API MediaMetaData : public Dictionary {
    public:
        using Dictionary::Dictionary;

        static constexpr auto Title = "title";

        QString title() const {
            return value(Title);
        }

        static MediaMetaData fromAVDictionary(const AVDictionary *dict) {
            MediaMetaData d;
            d.load(dict);
            return d;
        }
};

class NEKO_API GraphicsVideoItem : public QGraphicsObject {
    Q_OBJECT
    Q_PROPERTY(VideoSink* videoSink READ videoSink CONSTANT)
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
    Q_PROPERTY(VideoSink* videoSink READ videoSink CONSTANT)
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
    Q_PROPERTY(QString subtitleText READ subtitleText WRITE setSubtitleText NOTIFY subtitleTextChanged)
    Q_PROPERTY(QSize videoSize READ videoSize NOTIFY videoSizeChanged)
    public:
        explicit VideoSink(QObject *parent = nullptr);
        ~VideoSink();

        void setVideoFrame(const VideoFrame &frame);
        void setSubtitleText(const QString &subtitle);
        void addPixelFormat(VideoPixelFormat pixelFormat);
        VideoFrame videoFrame() const;
        QSize      videoSize() const;
        QString    subtitleText() const;
        QList<VideoPixelFormat> supportedPixelFormats() const;
    Q_SIGNALS:
        void videoFrameChanged(const VideoFrame &frame);
        void videoSizeChanged();
        void subtitleTextChanged(const QString &);
        void aboutToDestroy();
    private:
        QString    subtitle;
        VideoFrame frame;
        QSize      size = {0, 0};
        QList<VideoPixelFormat> formats; //< supported formats (default has RGBA32)
        std::shared_ptr<bool> mark = std::make_shared<bool>(true);
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
        void aboutToDestroy();
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

        QList<MediaMetaData> audioTracks() const;
        QList<MediaMetaData> videoTracks() const;
        QList<MediaMetaData> subtitleTracks() const;

        int activeAudioTrack() const;
        int activeVideoTrack() const;
        int activeSubtitleTrack() const;

        void setActiveAudioTrack(int index);
        void setActiveVideoTrack(int index);
        void setActiveSubtitleTrack(int index);

        void setAudioOutput(AudioOutput *output);
        AudioOutput *audioOutput() const;

        void setVideoOutput(QObject *);
        QObject *videoOutput() const;

        void setVideoSink(VideoSink *sink);
        VideoSink *videoSink() const;

        QUrl source() const;
        const QIODevice *sourceDevice() const;

        PlaybackState playbackState() const;
        MediaStatus mediaStatus() const;

        qreal duration() const;
        qreal position() const;

        bool hasAudio() const;
        bool hasVideo() const;

        float bufferProgress() const;
        qreal bufferedDuration() const;
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

        MediaMetaData metaData() const;

        void setOption(const QString &key, const QString &value);
        void clearOptions();

        void setHttpUseragent(const QString &useragent);
        void setHttpReferer(const QString &referer);

        void setInputFormat(AVInputFormat *avInputFormat);

        static QStringList supportedMediaTypes();
        static QStringList supportedProtocols();
    public Q_SLOTS:
        void play();
        void pause();
        void stop();
        void load();

        void setPosition(qreal position);

        void setPlaybackRate(qreal rate);

        void setSource(const QUrl &source);
        void setSourceDevice(QIODevice *device, const QUrl &sourceUrl = QUrl());
    Q_SIGNALS:
        void sourceChanged(const QUrl &media);
        void playbackStateChanged(PlaybackState newState);
        void mediaStatusChanged(MediaStatus status);

        void durationChanged(qreal duration);
        void positionChanged(qreal position);

        void hasAudioChanged(bool available);
        void hasVideoChanged(bool videoAvailable);

        void bufferProgressChanged(float progress);
        void bufferedDurationChanged(qreal duration);

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

inline size_t GetBytesPerSample(AudioSampleFormat fmt) {
    switch (fmt) {
        case AudioSampleFormat::Uint8: return sizeof(uint8_t);
        case AudioSampleFormat::Sint16: return sizeof(int16_t);
        case AudioSampleFormat::Sint32: return sizeof(uint32_t);
        case AudioSampleFormat::Float32: return sizeof(float);
    }
}
inline size_t GetBytesPerFrame(AudioSampleFormat fmt, int channels) {
    return GetBytesPerSample(fmt) * channels;
}

}

// Export namespace
NEKO_USING(GraphicsVideoItem);
NEKO_USING(AudioSampleFormat);
NEKO_USING(VideoPixelFormat);
NEKO_USING(MediaMetaData);
NEKO_USING(VideoFrame);
NEKO_USING(VideoSink);
NEKO_USING(VideoWidget);
NEKO_USING(MediaPlayer);
NEKO_USING(AudioOutput);

