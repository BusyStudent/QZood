#include "nekofmt.hpp"
#include <QDebug>
#include <QUrl>

extern "C" {
    #include <libavformat/avformat.h>
    #include <libavformat/version.h>
    #include <libavformat/avio.h>
}

#define CHECK_VERSION()                                  \
    if (avformat_version() != LIBAVFORMAT_VERSION_INT) { \
        qWarning() << "Build version was not same as runtime versoon"; \
    }                                                                  \

namespace NekoAV {

class DashInputFormatPrivate : public AVInputFormat {
    public:
        DashInputFormatPrivate() {
            CHECK_VERSION();

            ::memset(static_cast<AVInputFormat*>(this), 0, sizeof(AVInputFormat));

            name = "DashInputFormatPrivate";
            long_name = "nekoav DashInputFormat";
            flags = AVFMT_NOFILE | AVFMT_SEEK_TO_PTS;

            // Set functions
            read_probe = [](const AVProbeData *) {
                return 1;
            };
            read_header = [](AVFormatContext *ctxt) {
                return from(ctxt)->readHeader(ctxt);
            };
            read_packet = [](AVFormatContext *ctxt, AVPacket *packet) {
                return from(ctxt)->readPacket(ctxt, packet);
            };
            read_seek = [](AVFormatContext *ctxt, int stream_index, int64_t timestamp, int flags) {
                return from(ctxt)->readSeek(ctxt, stream_index, timestamp, flags);
            };
            read_close = [](AVFormatContext *ctxt) {
                return from(ctxt)->readClose(ctxt);
            };
        }
        ~DashInputFormatPrivate() {
            readClose(nullptr);

            av_dict_free(&videoOptions);
            av_dict_free(&audioOptions);
        }
    private:
        static auto from(AVFormatContext *ctxt) -> DashInputFormatPrivate * {
            auto self = static_cast<DashInputFormatPrivate *>(
                const_cast<AVInputFormat*>(ctxt->iformat)
            );
            assert(self->formatMagic = Magic);
            return self;
        }
        

        int readHeader(AVFormatContext *ctxt) {
            int ret;
            ret = avformat_open_input(&videoFormatCtxt, videoSource.toString().toUtf8().data(), nullptr, nullptr);
            if (ret < 0) {
                return ret;
            }
            ret = avformat_find_stream_info(videoFormatCtxt, nullptr);
            if (ret < 0) {
                return ret;
            }

            ret = avformat_open_input(&audioFormatCtxt, audioSource.toString().toUtf8().data(), nullptr, nullptr);
            if (ret < 0) {
                return ret;
            }
            ret = avformat_find_stream_info(audioFormatCtxt, nullptr);

            // Find audio stream
            audioStream = av_find_best_stream(audioFormatCtxt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
            videoStream = av_find_best_stream(videoFormatCtxt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

            // Check them
            if (videoStream < 0 || audioStream < 0) {
                return AVERROR_STREAM_NOT_FOUND;
            }
            parentVideoStream = avformat_new_stream(ctxt, nullptr);
            parentAudioStream = avformat_new_stream(ctxt, nullptr);


#if         1
            parentVideoStream->duration = videoFormatCtxt->streams[videoStream]->duration;
            parentAudioStream->duration = audioFormatCtxt->streams[audioStream]->duration;
            
            parentVideoStream->time_base = videoFormatCtxt->streams[videoStream]->time_base;
            parentAudioStream->time_base = audioFormatCtxt->streams[audioStream]->time_base;

            parentVideoStream->nb_frames = videoFormatCtxt->streams[videoStream]->nb_frames;
            parentAudioStream->nb_frames = audioFormatCtxt->streams[audioStream]->nb_frames;
#endif

            currentFormatCtxt = audioFormatCtxt;
            return 0;
        }
        int readPacket(AVFormatContext *ctxt, AVPacket *packet) {
            auto switchContext = [this]() {
                if (currentFormatCtxt == audioFormatCtxt) {
                    currentFormatCtxt = videoFormatCtxt;
                }
                else {
                    currentFormatCtxt = audioFormatCtxt;
                }
            };
            auto sendPacket = [this](AVPacket *packet) {
                if (currentFormatCtxt == audioFormatCtxt && packet->stream_index == audioStream) {
                    // Map to parent audio stream
                    packet->stream_index = parentAudioStream->index;
                }
                else if (currentFormatCtxt == videoFormatCtxt && packet->stream_index == videoStream) {
                    // Map to parent video stream
                    packet->stream_index = parentVideoStream->index;
                }
                else {
                    // Not us
                    av_packet_unref(packet);
                    return false;
                }
                return true;
            };
            while (true) {
                int ret = av_read_frame(currentFormatCtxt, packet);
                if (ret < 0) {
                    if (ret != AVERROR_EOF) {
                        // Error
                        return ret;
                    }
                    // Just eof switch to another ctxt
                    switchContext();
                    ret = av_read_frame(currentFormatCtxt, packet);
                    if (ret < 0) {
                        // Eof or something
                        return ret;
                    }
                    else {
                        if (!sendPacket(packet)) {
                            continue;
                        }
                    }
                }
                else {
                    if (!sendPacket(packet)) {
                        continue;
                    }
                }
                return ret;
            }
        }
        int readSeek(AVFormatContext *ctxt, int stream_index, int64_t timestamp, int flags) {
            if (stream_index == parentAudioStream->index) {
                return av_seek_frame(audioFormatCtxt, audioStream, timestamp, flags);
            }
            if (stream_index == parentVideoStream->index) {
                return av_seek_frame(videoFormatCtxt, videoStream, timestamp, flags);
            }
            return AVERROR_STREAM_NOT_FOUND;
        }
        int readClose(AVFormatContext *ctxt) {
            // Cleanup
            avformat_close_input(&videoFormatCtxt);
            avformat_close_input(&audioFormatCtxt);
            currentFormatCtxt = nullptr;

            audioStream = -1;
            videoStream = -1;

            parentAudioStream = nullptr;
            parentVideoStream = nullptr;

            return 0;
        }

        static constexpr int Magic = 0x114514;
        int              formatMagic = Magic;
        AVFormatContext *videoFormatCtxt = nullptr;
        AVFormatContext *audioFormatCtxt = nullptr;

        AVFormatContext *currentFormatCtxt = nullptr;
        
        QUrl             videoSource;
        QUrl             audioSource;
        
        AVDictionary    *videoOptions = nullptr;
        AVDictionary    *audioOptions = nullptr;

        int              audioStream = -1;
        int              videoStream = -1;

        AVStream        *parentAudioStream = nullptr;
        AVStream        *parentVideoStream = nullptr;
    friend class DashInputFormat;
};

DashInputFormat::DashInputFormat(QObject *parent) : QObject(parent), d(new DashInputFormatPrivate()) {

}
DashInputFormat::~DashInputFormat() {

}

void *DashInputFormat::getAVInputFormat() {
    return static_cast<AVInputFormat*>(d.get());
}
void  DashInputFormat::setVideoSource(const QUrl &source) {
    d->videoSource = source;
}
void  DashInputFormat::setAudioSource(const QUrl &source) {
    d->audioSource = source;
}
void  DashInputFormat::setVideoOption(const QByteArray &key, const QByteArray &value) {
    av_dict_set(&d->videoOptions, key.data(), value.data(), 0);
}
void  DashInputFormat::setAudioOption(const QByteArray &key, const QByteArray &value) {
    av_dict_set(&d->audioOptions, key.data(), value.data(), 0);
}
void  DashInputFormat::clearAudioOption() {
    av_dict_free(&d->audioOptions);
}
void  DashInputFormat::clearVideoOption() {
    av_dict_free(&d->videoOptions);
}

}