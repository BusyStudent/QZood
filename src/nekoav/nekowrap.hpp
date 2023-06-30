#pragma once

#include <QString>

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
class AVTraits<AVSubtitle> {
    public:
        void operator()(AVSubtitle *ptr) {
            avsubtitle_free(ptr);
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
class AVPtr : public std::unique_ptr<T, AVTraits<T> > {
    public:
        using Base = std::unique_ptr<T, AVTraits<T> >;
        using Base::unique_ptr;
};

class AVError {
    public:
        AVError(int errcode = 0) : _errcode(errcode) { }
        AVError(const AVError &) = default;

        bool ok() const noexcept {
            return _errcode >= 0;
        }
        bool bad() const noexcept {
            return _errcode < 0;
        }
        int  code() const noexcept {
            return _errcode;
        }
        QString message() const {
            char buffer[AV_ERROR_MAX_STRING_SIZE];
            return av_make_error_string(buffer, sizeof(buffer), _errcode);
        }

        operator bool() const noexcept {
            return ok();
        }
        operator int() const noexcept {
            return _errcode;
        }
    private:
        int _errcode;
};

// Helper function
inline int64_t doubelToFFInt64(qreal d) noexcept {
    return d * 1000000.0;
}
inline qreal   FFInt64ToDouble(int64_t t) noexcept {
    return t / 1000000.0;
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

/**
 * @brief Create a basic decoder object
 * 
 * @param stream The AVStream pointer (can not be nullptr)
 * @return std::pair<AVCodecContext*, int> 
 */
inline std::pair<AVCodecContext*, int> FFCreateDecoderContext(AVStream *stream) {
    auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    int errcode = 0;
    if (!codec) {
        // No codec
        errcode = AVERROR_DECODER_NOT_FOUND;
        return {nullptr, errcode};
    }
    auto codecCtxt = avcodec_alloc_context3(codec);
    if (!codec) {
        errcode = AVERROR(ENOMEM);
        return {nullptr, errcode};
    }
    errcode = avcodec_parameters_to_context(codecCtxt, stream->codecpar);
    if (errcode < 0) {
        avcodec_free_context(&codecCtxt);
        return {nullptr, errcode};
    }
    errcode = avcodec_open2(codecCtxt, codecCtxt->codec, nullptr);
    if (errcode < 0) {
        avcodec_free_context(&codecCtxt);
        return {nullptr, errcode};
    }
    return {codecCtxt, errcode};
}

}