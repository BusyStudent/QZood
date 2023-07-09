#include "nekoutils.hpp"
#include "nekowrap.hpp"
#include <memory>

// Import platform specific
#if defined(_WIN32)
    #include <windows.h>
    #include <ShlObj.h>
    #include <wrl.h>
#endif



namespace NekoAV {

QImage GetMediaFileIcon(const QString &filename) {

#if defined(_WIN32)
// #if 0
    using Microsoft::WRL::ComPtr;
    auto winfilename = filename;
    winfilename.replace("/", "\\");

    auto buffer = std::make_unique<wchar_t []>(winfilename.length() + 1);
    auto n = winfilename.toWCharArray(buffer.get());
    buffer[n] = L'\0';

    ComPtr<IShellItemImageFactory> factory;
    auto hr = ::SHCreateItemFromParsingName(buffer.get(), nullptr, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return QImage();
    }
    SIZE    size   = {192, 108};
    HBITMAP bitmap = nullptr;
    hr = factory->GetImage(size, SIIGBF_BIGGERSIZEOK, &bitmap);
    if (FAILED(hr)) {
        return QImage();
    }
    auto image = QImage::fromHBITMAP(bitmap);
    ::DeleteObject(bitmap);
    return image;
#else
    // AVPtr<AVFormatContext> formatContext;
    // AVPtr<AVCodecContext>  codecContext;
    // AVPtr<AVPacket>        packet {av_packet_alloc()};
    // AVPtr<AVFrame>         frame {av_frame_alloc()};
    // int errcode = avformat_open_input(&formatContext, filename.toUtf8().constData(), nullptr, nullptr);
    // if (errcode < 0) {
    //     return QImage();
    // }
    // errcode = avformat_find_stream_info(formatContext.get(), nullptr);
    // if (errcode < 0) {
    //     return QImage();
    // }
    // int videoStream = av_find_best_stream(formatContext.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    // if (videoStream < 0) {
    //     return QImage();
    // }
    // auto [createCodecContext, createErrcode] = FFCreateDecoderContext(formatContext->streams[videoStream]);
    // if (createErrcode < 0) {
    //     return QImage();
    // }
    // codecContext.reset(createCodecContext);

    // // Set AVDISCARD_ALL 	
    // for (int i = 0; i < formatContext->nb_streams; i++) {
    //     if (i != videoStream) {
    //         formatContext->streams[i]->discard = AVDISCARD_ALL;
    //     }
    // }

    // errcode = av_read_frame(formatContext.get(), packet.get());
    // if (errcode < 0) {
    //     return QImage();
    // }
    // errcode = avcodec_send_packet(codecContext.get(), packet.get());
    // av_packet_unref(packet.get());
    // if (errcode < 0) {
    //     return QImage();
    // }
    // errcode = avcodec_receive_frame(codecContext.get(), frame.get());
    // if (errcode < 0) {
    //     return QImage();
    // }

    // int width = frame->width;
    // int height = frame->height;

    // TODO : Convert to RGBA and write

    return QImage();
#endif

}


}