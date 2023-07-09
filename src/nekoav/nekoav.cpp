#define NEKO_SOURCE
#include "nekoav.hpp"
#include "nekoprivate.hpp"
#include <QAbstractEventDispatcher>
#include <QRegularExpression>
#include <QIODevice>
#include <thread>

#define NEKOAV_TIME_BASE 1000000.0

namespace NekoAV {

// Packet Queue Part
PacketQueue::PacketQueue() = default;
PacketQueue::~PacketQueue() {
    flush();
}

void PacketQueue::put(AVPacket *packet) {
    if (!packet) {
        return;
    }
    std::lock_guard locker(mutex);
    packets.push_back(packet);
    // Sums packet to let us known the buffered video
    if (!IsSpecialPacket(packet)) {
        packetsDuration += packet->duration;
    }
    cond.notify_one();
}
void PacketQueue::unget(AVPacket *packet) {
    std::lock_guard locker(mutex);
    packets.push_front(packet);
    // Sums packet to let us known the buffered video
    if (!IsSpecialPacket(packet)) {
        packetsDuration += packet->duration;
    }
    cond.notify_one();
}
void PacketQueue::flush() {
    std::lock_guard locker(mutex);
    while (!packets.empty()) {
        auto p = packets.front();
        if (!IsSpecialPacket(p)) {
            av_packet_free(&p);
        }
        packets.pop_front();
    }
    packetsDuration = 0;
}
bool PacketQueue::stopRequested() const {
    return stop;
}
void PacketQueue::requestStop() {
    stop = true;
    cond.notify_all();
}

size_t PacketQueue::size() const {
    std::lock_guard locker(mutex);
    return packets.size();
}
int64_t PacketQueue::duration() const {
    std::lock_guard locker(mutex);
    return packetsDuration;
}

AVPacket *PacketQueue::get(bool block) {
    AVPacket *ret = nullptr;
    mutex.lock();
    while (packets.empty()) {
        mutex.unlock();
        if (!block) {
            return nullptr;
        }
        if (stop) {
            return nullptr;
        }


        qDebug() << "PacketQueue waiting for more packets...";

        std::unique_lock lock(condMutex);
        cond.wait(lock);

        mutex.lock();
    }
    ret = packets.front();
    packets.pop_front();
    if (!IsSpecialPacket(ret)) {
        packetsDuration -= ret->duration;
    }

    mutex.unlock();

    return ret;
}

// AudioThread    
AudioThread::AudioThread(DemuxerThread *parent, AVStream *stream, AVCodecContext *ctxt) 
    : QObject(), 
      stream(stream), 
      codecCtxt(ctxt),
      audioOutput(parent->audioOutput())
{
    outputSampleFormat = AudioSampleFormat::Float32;
    outputSampleRate = ctxt->sample_rate;
    outputChannels = ctxt->channels;
    needResample = false;
    switch (ctxt->sample_fmt) {
        case AV_SAMPLE_FMT_U8 : outputSampleFormat = AudioSampleFormat::Uint8; break;
        case AV_SAMPLE_FMT_S16 : outputSampleFormat = AudioSampleFormat::Sint16; break;
        // case AV_SAMPLE_FMT_S32 : outputSampleFormat = AudioSampleFormat::Sint32; break;
        // May has S24 but input like AV_SAMPLE_FMT_S32
        // I didnot how to handle it, so just convert
        case AV_SAMPLE_FMT_FLT : outputSampleFormat = AudioSampleFormat::Float32; break;
        default : needResample = true; outputSampleFormat = AudioSampleFormat::Float32; break;
    }
    if (!needResample) {
        qDebug() << "AudioThread: output supported format, passthrough";
    }


    audioOutput->setCallback([this](void *data, int n) {
        audioCallback(data, n);
    });   
    audioOutput->open(
        outputSampleFormat,
        outputSampleRate,
        outputChannels
    );
    audioOutput->pause(true);
    audioInitialized = audioOutput->isOpen();
}
AudioThread::~AudioThread() {
    audioOutput->close();

    avcodec_free_context(&codecCtxt);
}
void AudioThread::pause(bool v) {
    audioOutput->pause(v);
}
// FIXME : Memory bug here
void AudioThread::audioCallback(void *data, int len) {
    uint8_t *dst = static_cast<uint8_t*>(data);
    while (len > 0) {
        if (bufferIndex >= bufferSize) {
            // Run out of buffer
            int audioSize = audioDecodeFrame();
            if (audioSize < 0) {
                // No audio
                break;
            }
        }

        // Write buffer
        int left = bufferSize - bufferIndex;
        left = qMin(len, left);

        ::memcpy(dst, buffer + bufferIndex, left);
        
        dst += left;
        
        len -= left;
        bufferIndex += left;

        // Update current clock
        audioClock = audioClock + qreal(left) / GetBytesPerFrame(outputSampleFormat, outputChannels) / outputSampleRate; 
    }

    // Make slience
    if (len) {
        ::memset(dst, 0, len);
    }
}
int AudioThread::audioDecodeFrame() {
    // Try get packet
    int ret;
    while (true) {
        AVPacket *packet = queue.get(false);
        if (packet == EofPacket) {
            // No more data
            waitting = true;
            return -1;
        }
        if (packet == FlushPacket) {
            avcodec_flush_buffers(codecCtxt);
            swrCtxt.reset();

            // BTK_LOG(BTK_RED("[AudioThread] ") "Got flush\n");
            continue;
        }
        if (packet == nullptr) {
            // No Data
            waitting = true;
            return -1;
        }
        waitting = false;

        // Normal data
        AVPtr<AVPacket> guard(packet);

        // Check the timestamp here
        int64_t curTime = av_gettime_relative();

        ret = avcodec_send_packet(codecCtxt, packet);
        if (ret < 0) {
            // Too much
            if (ret != AVERROR(EAGAIN)) {
                return -1;
            }
        }
        ret = avcodec_receive_frame(codecCtxt, frame.get());
        if (ret < 0) {
            if (ret == AVERROR(EAGAIN)) {
                // We need more
                continue;
            }
            return -1; // Err
        }

        double decodeUsed = (av_gettime_relative() - curTime) / NEKOAV_TIME_BASE;
        if (decodeUsed > 0.04) {
            qWarning() << "AudioThread spent too long to decode" << decodeUsed;
        }

        // Update audio clock 
        if (frame->pts == AV_NOPTS_VALUE) {
            audioClock = 0.0;
        }
        else {
            audioClock = frame->pts * av_q2d(stream->time_base);
            // BTK_LOG("Audio clock %lf\n", audio_clock);
        }

        // Got it
        // int size = audioResample(audio_sync(frame->nb_samples));
        int size = audioResample(frame->nb_samples);
        if (size < 0) {
            return size;
        }

        return size;
    }
}
int AudioThread::audioResample(int wanted_samples) {
    if (!needResample) {
        // Just output this data
        int size = frame->linesize[0];

        buffer = frame->data[0];
        bufferSize = size;
        bufferIndex = 0;

        return size;
    }

    // Get frame samples buffer size
    int data_size = av_samples_get_buffer_size(nullptr, codecCtxt->channels, frame->nb_samples, AVSampleFormat(frame->format), 1);

    auto out_sample_rate = codecCtxt->sample_rate;
    auto in_sample_rate = frame->sample_rate;

    if (!swrCtxt) {
        // init swr context
        int64_t in_channel_layout = (codecCtxt->channels ==
                            av_get_channel_layout_nb_channels(codecCtxt->channel_layout)) ?
                            codecCtxt->channel_layout :
                            av_get_default_channel_layout(codecCtxt->channels);
        auto    out_chanel_layout = in_channel_layout;

        Q_ASSERT(frame->format == codecCtxt->sample_fmt);

        swrCtxt.reset(
            swr_alloc_set_opts(
                nullptr,
                out_chanel_layout,
                AV_SAMPLE_FMT_FLT,
                out_sample_rate,
                in_channel_layout,
                codecCtxt->sample_fmt,
                in_sample_rate,
                0,
                nullptr
            )
        );
        if (!swrCtxt || swr_init(swrCtxt.get()) < 0) {
            qDebug() << ("swr_alloc_set_opts() failed\n");
            return -1;
        }
    }
    if (swrCtxt) {
        // Convert
        uint8_t *buffer_data;

        const uint8_t **in = (const uint8_t**) frame->data;
        uint8_t **out = &buffer_data;

        int out_count = int64_t(wanted_samples) * out_sample_rate / in_sample_rate;
        int out_size = av_samples_get_buffer_size(nullptr, codecCtxt->channels, out_count, codecCtxt->sample_fmt, 1);
        if (out_size < 0) {
            qDebug() << ("av_samples_get_buffer_size() failed\n");
            return -1;
        }
        if (wanted_samples != frame->nb_samples) {
            if (swr_set_compensation(swrCtxt.get(), (wanted_samples - frame->nb_samples) * out_sample_rate / frame->sample_rate,
                                                    wanted_samples * out_sample_rate / frame->sample_rate) < 0) {
                qDebug() << ("swr_set_compensation failed\n");
                return -1;
            }
        }

        FFReallocateBuffer(&swrBuffer, out_size);

        buffer = swrBuffer.get();
        buffer_data = swrBuffer.get();

        int len2 = swr_convert(swrCtxt.get(), out, out_count, in, frame->nb_samples);
        if (len2 < 0) {
            qDebug() << ("swr_convert failed\n");
            return -1;
        }
        if (len2 == out_count) {
            // BTK_LOG("audio buffer is probably too small\n");
        }
        int resampled_data_size = len2 * codecCtxt->channels * av_get_bytes_per_sample(codecCtxt->sample_fmt);

        buffer = swrBuffer.get();
        bufferIndex = 0;
        bufferSize = resampled_data_size;
        return resampled_data_size;
    }
    else {
        // Opps
        qDebug() << ("swr_alloc_set_opts() failed\n");
        return -1;
    }
}


// VideoThread
VideoThread::VideoThread(DemuxerThread *parent, AVStream *stream, AVCodecContext *ctxt) :
    demuxerThread(parent),
    stream(stream),
    codecCtxt(ctxt)    
{
    tryHardwareInit();
    setObjectName("NekoAV VideoThread");
    
    videoSink = demuxerThread->videoSink();

    pause(true);

    // Create the video thread
    presentThread = QThread::create(&VideoThread::run, this);
    presentThread->setObjectName("NekoAV VideoPresentThread");
    presentThread->start();
}
VideoThread::~VideoThread() {
    queue.flush();
    queue.requestStop();
    pause(false);
    // Wait
    presentThread->wait();
    delete presentThread;

    avcodec_free_context(&codecCtxt);
}
void VideoThread::tryHardwareInit() {
    AVHWDeviceType hardwareType;

    // Try create a context with hardware
    auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    AVPtr<AVCodecContext> hardwareCtxt(avcodec_alloc_context3(codec));
    if (!hardwareCtxt) {
        return;
    }
    if (avcodec_parameters_to_context(hardwareCtxt.get(), stream->codecpar) < 0) {
        return;
    }

    // Get Configuaration
    const AVCodecHWConfig *conf = nullptr;
    for (int i = 0; ; i++) {
        conf = avcodec_get_hw_config(codec, i);
        if (!conf) {
            return;
        }

        if (!(conf->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX)) {
            continue;
        }
        // Got
        hardwarePixfmt = conf->pix_fmt;
        hardwareType = conf->device_type;

        // Override HW get format
        hardwareCtxt->opaque = this;
        hardwareCtxt->get_format = [](struct AVCodecContext *s, const enum AVPixelFormat * fmt) {
            auto self = static_cast<VideoThread*>(s->opaque);
            auto p = fmt;
            while (*p != -1) {
                if (*p == self->hardwarePixfmt) {
                    return self->hardwarePixfmt;
                }
                if (*(p + 1) == AV_PIX_FMT_NONE) {
                    qWarning() << "Cannot get hardware format, fallback to " << av_pix_fmt_desc_get(*p)->name;
                    return *p;
                }
                p ++;
            }
            // Almost impossible, first is none pixfmt
            abort();
        };

        // Try init hw
        AVBufferRef *hardwareDeviceCtxt = nullptr;
        if (av_hwdevice_ctx_create(&hardwareDeviceCtxt, hardwareType, nullptr, nullptr, 0) < 0) {
            // Failed
            continue;
        }
        // hardwareCtxt->hw_device_ctx = av_buffer_ref(hardwareDeviceCtxt);
        hardwareCtxt->hw_device_ctx = hardwareDeviceCtxt;

        // Init codec
        if (avcodec_open2(hardwareCtxt.get(), codec, nullptr) < 0) {
            continue;
        }

        // Done
        avcodec_free_context(&codecCtxt);
        codecCtxt = hardwareCtxt.release();

        qDebug() << "VideoThread tryHardwareInit ok " << av_hwdevice_get_type_name(hardwareType) 
                << " hardwareFormat " << av_pix_fmt_desc_get(hardwarePixfmt)->name;

        return;
    }
}
void VideoThread::run() {
    videoClockStart = av_gettime_relative();
    swsScaleDuration = 0.0;
    // Try get packet
    int ret;
    while (!queue.stopRequested()) {
        if (paused) {
            std::unique_lock lock(condMutex);
            cond.wait(lock);
            continue;
        }
        AVPacket *packet = queue.get();
        if (packet == EofPacket) {
            // No more data
            waitting = true;
            continue;
        }
        if (packet == FlushPacket) {
            avcodec_flush_buffers(codecCtxt);

            // BTK_LOG(BTK_RED("[VideoThread] ") "Got flush\n");
            continue;
        }
        if (packet == nullptr) {
            // No Data
            waitting = true;
            continue;
        }
        waitting = false;

        // Let's begin
        AVPtr<AVPacket> guard(packet);
        AVFrame *frame;
        if (!videoDecodeFrame(packet, &frame)) {
            continue;
        }

        // TODO : Add sws_scale_duration to adjust the time
        // Sync
        double currentFramePts = srcFrame->pts * av_q2d(stream->time_base);
        double masterClock = demuxerThread->clock();
        double diff = masterClock - videoClock - swsScaleDuration - videoDecodeDuration;

        videoClock = currentFramePts;
        videoFrameCount += 1;

        if (diff < 0 && -diff < AVNoSyncThreshold) {
            // We are too fast
            auto delay = -diff;

            std::unique_lock lock(condMutex);
            cond.wait_for(lock, std::chrono::milliseconds(int64_t(delay * 1000)));
        }
        else if (diff > 0.3) {
            // We are too slow, drop
            // BTK_LOG(BTK_RED("[VideoThread] ") "A-V = %lf Too slow, drop sws_duration = %lf\n", diff, sws_scale_duration);
            videoDropedFrameCount += 1;
            qDebug() << "VideoThread drop frame for " << videoDropedFrameCount << " / " << videoFrameCount;
            continue;
        }
        else if (diff < AVSyncThreshold) {
            // Sleep we we should
            double delay = 0;
            if (srcFrame->pkt_duration != AV_NOPTS_VALUE) {
                delay = srcFrame->pkt_duration * av_q2d(stream->time_base);
            }
            // BTK_LOG("duration %lf, diff %lf\n", delay, diff);
            // Sleep for it
            delay = qMin(delay, diff);

            std::unique_lock lock(condMutex);
            cond.wait_for(lock, std::chrono::milliseconds(int64_t(delay * 1000)));
        }

        videoWriteFrame(frame);
    }
    videoSink->setVideoFrame(VideoFrame());
}
bool VideoThread::videoDecodeFrame(AVPacket *packet, AVFrame **retFrame) {
    int64_t decBeginTime = av_gettime_relative();

    AVFrame *cvtSource = srcFrame.get();
    int ret;
    ret = avcodec_send_packet(codecCtxt, packet);
    if (ret < 0) {
        // BTK_LOG(BTK_RED("[VideoThread] ") "avcodec_send_packet failed!!!\n");
        return false;
    }
    ret = avcodec_receive_frame(codecCtxt, srcFrame.get());
    if (ret < 0) {
        // Need more packet
        return false;
    }
    if (srcFrame->format == hardwarePixfmt) {
        // Hardware decode
        ret = av_hwframe_transfer_data(swFrame.get(), srcFrame.get(), 0);
        if (ret < 0) {
            return false;
        }
        cvtSource = swFrame.get();
    }

    *retFrame = cvtSource;
    videoDecodeDuration = (av_gettime_relative() - decBeginTime) / NEKOAV_TIME_BASE;

    return true;
}
void VideoThread::videoWriteFrame(AVFrame *source) {
    
    // Lazy eval beacuse of the hardware access
    if (firstFrame) {
        firstFrame = false;

        // Detect formats
        auto srcFormat = AVPixelFormat(source->format);
        needConvert = !videoSink->supportedPixelFormats().contains(ToVideoPixelFormat(srcFormat));

        qDebug() << "VideoThread source frame format" << av_pix_fmt_desc_get(srcFormat)->name;

        if (needConvert) {
            // Prepare convertion buffer
            auto dstFormat = ToAVPixelFormat(videoSink->supportedPixelFormats().first());

            size_t n     = av_image_get_buffer_size(dstFormat, codecCtxt->width, codecCtxt->height, 32);
            uint8_t *buf = static_cast<uint8_t*>(av_malloc(n));

            av_image_fill_arrays(
                dstFrame->data,
                dstFrame->linesize,
                buf,
                dstFormat,
                codecCtxt->width, codecCtxt->height,
                32
            );

            swsCtxt.reset(
                sws_getContext(
                    codecCtxt->width,
                    codecCtxt->height,
                    AVPixelFormat(source->format),
                    codecCtxt->width,
                    codecCtxt->height,
                    dstFormat,
                    0,
                    nullptr,
                    nullptr,
                    nullptr
                )
            );
            if (!swsCtxt) {
                // BTK_LOG(BTK_RED("[VideoThread] ") "sws_getContext failed!!!\n");
                return;
            }
            dstFrame->format = dstFormat;
        }
        else {
            qDebug() << "VideoSink directly support" << av_pix_fmt_desc_get(srcFormat)->name << " ,just passthrough";
        }
    }

    if (!needConvert) {
        swsScaleDuration = 0.0;

        videoSink->setVideoFrame(VideoFrame::fromAVFrame(source));
    }
    else if (swsCtxt) {
        // Need convert and has ctxt
        // Convert it
        std::lock_guard locker(dstFrameMutex);
        
        int64_t swsBeginTime = av_gettime_relative();
        int ret = sws_scale(
            swsCtxt.get(),
            source->data,
            source->linesize,
            0,
            codecCtxt->height,
            dstFrame->data,
            dstFrame->linesize
        );

        // Copy dara
        dstFrame->width = codecCtxt->width;
        dstFrame->height = codecCtxt->height;

        swsScaleDuration = (av_gettime_relative() - swsBeginTime) / NEKOAV_TIME_BASE;

        if (ret < 0) {
            // BTK_LOG(BTK_RED("[VideoThread] ") "sws_scale failed %d!!!\n", ret);
            return;
        }

        videoSink->setVideoFrame(VideoFrame::fromAVFrame(dstFrame.get()));
    }
}
void VideoThread::pause(bool v) {
    if (paused == v) {
        return;
    }
    paused = v;
    cond.notify_one();
}

SubtitleThread::SubtitleThread(DemuxerThread *parent, AVStream *stream, AVCodecContext *ctxt) :
    QThread(),
    demuxerThread(parent),
    stream(stream),
    codecCtxt(ctxt)    
{
    setObjectName("NekoAV SubtitleThread");
    
    videoSink = demuxerThread->videoSink();

    start();
    pause(true);
}

SubtitleThread::~SubtitleThread() {
    queue.flush();
    queue.requestStop();
    pause(false);
    cond.notify_one();
    // Wait
    wait();

    avcodec_free_context(&codecCtxt);
}
void SubtitleThread::run() {
    // Try get packet
    int ret;
    int got;

    AVSubtitle subtitle;
    QRegularExpression reg("{.*?}");
    QString text;
    while (!queue.stopRequested()) {
        mainloop : 
        if (reqRefresh) {
            reqRefresh = false;
        }
        if (paused) {
            waitting = true;
            std::unique_lock lock(condMutex);
            cond.wait(lock);
            continue;
        }
        AVPacket *packet = queue.get();
        if (packet == EofPacket) {
            // No more data
            waitting = true;
            continue;
        }
        if (packet == FlushPacket) {
            avcodec_flush_buffers(codecCtxt);
            
            // BTK_LOG(BTK_RED("[VideoThread] ") "Got flush\n");
            continue;
        }
        if (packet == nullptr) {
            // No Data
            waitting = true;
            continue;
        }
        waitting = false;

        AVPtr<AVPacket> pguard(packet);

        ret = avcodec_decode_subtitle2(codecCtxt, &subtitle, &got, packet);
        if (!got || ret < 0) {
            continue;
        }
        
        // Free the subtitle if
        AVPtr<AVSubtitle> sguard(&subtitle);

        if (subtitle.num_rects <= 0) {
            continue;
        }
        auto data = subtitle.rects[0];
        if (data->type == SUBTITLE_TEXT) {
            text = QString::fromUtf8(data->text);
        }
        if (data->type == SUBTITLE_ASS) {
            text = QString::fromUtf8(data->ass).replace(reg, "").split(",").back();
        }

        // Sleep until we got it
        double duration = packet->duration * av_q2d(stream->time_base);
        double currentPts = packet->pts * av_q2d(stream->time_base);
        double startPts = currentPts + subtitle.start_display_time / NEKOAV_TIME_BASE;
        double endPts = currentPts + subtitle.end_display_time / NEKOAV_TIME_BASE + duration;

        double diff = startPts - demuxerThread->clock();

        while (diff > 0) {
            // Wait until
            std::unique_lock lock(condMutex);
            cond.wait_for(lock, 10ms);

            diff = startPts - demuxerThread->clock();
            if (queue.stopRequested()) {
                goto quitLabel;
            }
            if (reqRefresh) {
                goto mainloop;
            }
        }
        if (demuxerThread->clock() > endPts) {
            // Skip it
            continue;
        }

        // Put subtitle
        qDebug() << "Puting subtitle " << text << "Started " << startPts << " end " << endPts;
        videoSink->setSubtitleText(text);

        diff = endPts - demuxerThread->clock();
        while (diff > 0) {
            // Wait until
            std::unique_lock lock(condMutex);
            cond.wait_for(lock, 10ms);

            diff = endPts - demuxerThread->clock();

            if (queue.stopRequested()) {
                goto quitLabel;
            }
            if (reqRefresh) {
                goto mainloop;
            }
        }

        // End 
        videoSink->setSubtitleText(QString());
    }

    quitLabel : videoSink->setSubtitleText(QString());
}
void SubtitleThread::pause(bool v) {
    if (paused == v) {
        return;
    }
    paused = v;
    cond.notify_one();
}
void SubtitleThread::switchStream(AVStream* stream) {
    refresh();

    while (!waitting) { 
        std::this_thread::sleep_for(1ms);
    }

    avcodec_free_context(&codecCtxt);
    auto [ctxt, errcode] = FFCreateDecoderContext(stream);
    codecCtxt = ctxt;
}
void SubtitleThread::refresh() {
    reqRefresh = true;
    queue.flush();
    queue.put(FlushPacket);
    cond.notify_one();
}

// Demuxer here
DemuxerThread::DemuxerThread(MediaPlayerPrivate *parent) : QThread(), player(parent) {
    setObjectName("NekoAV DemuxerThread");
}
DemuxerThread::~DemuxerThread() {
    // Tell it we should  quit
    quit = true;
    wakeUp();
    wait();

    avformat_close_input(&formatCtxt);
    av_free(ioBuffer);
}
void DemuxerThread::run() {
    QScopedPointer<QObject> invokeHelperGuard(new QObject());
    invokeHelper = invokeHelperGuard.get();
    invokeHelper->moveToThread(this);

    if (!load()) {
        return;
    }
    if (prepareWorker()) {
        runDemuxer();
    }
    
    // Settings some status
    Q_EMIT ffmpegPositionChanged(0.0);
    player->setMediaStatus(MediaStatus::EndOfMedia);
    player->setPlaybackState(PlaybackState::StoppedState);

    // Cleanup
    delete audioThread;
    delete videoThread;
    delete subtitleThread;

    audioThread = nullptr;
    videoThread = nullptr;
    subtitleThread = nullptr;
    invokeHelper = nullptr;

    avformat_close_input(&formatCtxt);
    av_packet_free(&packet);
}
bool DemuxerThread::load() {
    // Shoud we lock here ?
    std::lock_guard locker(player->settingsMutex);

    formatCtxt = avformat_alloc_context();
    if (!formatCtxt) {
        errcode = AVERROR(ENOMEM);
        return sendError(errcode);
    }

    // Set interrupt handler
    formatCtxt->interrupt_callback.callback = [](void *_self) -> int {
        return static_cast<DemuxerThread *>(_self)->interruptHandler();
    };
    formatCtxt->interrupt_callback.opaque = this;

    // Set ADIOContext if
    if (player->ioDevice) {
        auto ioDevice = player->ioDevice;

        // Alloc memory for 
        ioBufferSize = 32 * 1024; //< 32K
        ioBuffer = (uint8_t*) av_malloc(ioBufferSize);

        ioCtxt = avio_alloc_context(
            ioBuffer,
            ioBufferSize,
            ioDevice->isWritable(),
            ioDevice,
            ioDevice->isReadable() ? +[](void *opaque, uint8_t *buf, int buf_size) -> int {
                auto dev = static_cast<QIODevice *>(opaque);
                auto n = dev->read((char *)buf, buf_size);

                return n;
            } : nullptr,
            ioDevice->isWritable() ? +[](void *opaque, uint8_t *buf, int buf_size) -> int {
                auto dev = static_cast<QIODevice *>(opaque);
                auto n = dev->write((char*)buf, buf_size);

                return n;
            } : nullptr,
            !ioDevice->isSequential() ? +[](void *opaque, int64_t offset, int whence) -> int64_t {
                auto dev = static_cast<QIODevice *>(opaque);
                auto pos = dev->pos();                

                // Get Current pos;
                if (pos < 0) {
                    return -1;
                }
                switch (whence) {
                    case SEEK_SET : pos = offset; break;
                    case SEEK_CUR : pos += offset; break;
                    case SEEK_END : pos = dev->size() + offset; break;
                }
                if (dev->seek(pos)) {
                    return 0;
                }
                return -1;
            } : nullptr
        );
        formatCtxt->pb = ioCtxt;
    }

    // Try open
    QByteArray url;
    if (player->url.isLocalFile()) {
        url = player->url.toLocalFile().toUtf8();
    }
    else {
        url = player->url.toString().toUtf8();
    }

    player->setMediaStatus(MediaStatus::LoadingMedia);
    errcode = avformat_open_input(
        &formatCtxt,
        url.data(),
        player->inputFormat,
        &player->options
    );
    if (errcode < 0) {
        player->setMediaStatus(MediaStatus::InvalidMedia);
        return sendError(errcode);
    }
    
    // Begin get info
    errcode = avformat_find_stream_info(formatCtxt, nullptr);
    if (errcode < 0) {
        player->setMediaStatus(MediaStatus::InvalidMedia);
        return sendError(errcode);
    }

    // Dump info
    av_dump_format(formatCtxt, 0, url.data(), 0);

    player->setMediaStatus(MediaStatus::LoadedMedia);
    player->loaded = true;

    // Get Stream of it
    player->videoStream = av_find_best_stream(formatCtxt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    player->audioStream = av_find_best_stream(formatCtxt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    player->subtitleStream = av_find_best_stream(formatCtxt, AVMEDIA_TYPE_SUBTITLE, -1, -1, nullptr, 0);

    if (player->audioStream < 0 && player->videoStream < 0) {
        // No stream !!!
        errcode = AVERROR_STREAM_NOT_FOUND;
        return sendError(errcode);
    }

    Q_EMIT ffmpegMediaLoaded();

    // Check the URL if is network stream
    if (url.startsWith("http")) {
        // TODO : Add more checking
        isLocalSource = false;
    }
    else {
        isLocalSource = true;
    }

    // If not playing just load, waiting for it
    while (player->playbackState != PlaybackState::PlayingState) {
        waitForEvent(1h);
    }

    // Done
    return true;
}
bool DemuxerThread::prepareWorker() {
    if (player->audioStream >= 0 && audioOutput()) {
        if (!prepareCodec(player->audioStream)) {
            return false;
        }
    }
    if (player->videoStream >= 0 && videoSink()) {
        if (!prepareCodec(player->videoStream)) {
            return false;
        }
    }
    if (player->subtitleStream >= 0 && videoSink()) {
        if (!prepareCodec(player->subtitleStream)) {
            return false;
        }
    }
    return true;
}
bool DemuxerThread::prepareCodec(int streamid) {
    auto stream = formatCtxt->streams[streamid];
    auto [codecCtxt, retCode] = FFCreateDecoderContext(stream);
    if (codecCtxt == nullptr) {
        errcode = retCode;
        return sendError(errcode);
    }

    qDebug() << "DemuxerThread Create Codec " << codecCtxt->codec->name << " fullname" << codecCtxt->codec->long_name;

    // Common init done
    if (codecCtxt->codec_type == AVMEDIA_TYPE_AUDIO) {
        audioThread = new AudioThread(this, stream, codecCtxt);
        return true;
    }
    else if (codecCtxt->codec_type == AVMEDIA_TYPE_VIDEO) {
        videoThread = new VideoThread(this, stream, codecCtxt);
        return true;
    }
    else if (codecCtxt->codec_type == AVMEDIA_TYPE_SUBTITLE) {
        subtitleThread = new SubtitleThread(this, stream, codecCtxt);
        return true;
    }

    // Error or Unknown
    avcodec_free_context(&codecCtxt);
    errcode = AVERROR_UNKNOWN;
    return sendError(errcode);

}
bool DemuxerThread::runDemuxer() {
    if (packet == nullptr) {
        packet = av_packet_alloc();
    }

    externalClockStart = av_gettime_relative();

    // Start all worker
    doPause(false);

    int eof = false;
    while (true) {
        mainloop : doUpdateClock();

        if (quit) {
            break;
        }

        if (hasSeek) {
            if (!doSeek()) {
                // Seek Error
                qDebug() << "Demuxer Seek Error";
                return false;
            }

            eof = false;
        }
        if (player->playbackState == PlaybackState::PausedState) {
            // If paused state
            doPause(true);
            while (player->playbackState == PlaybackState::PausedState) {
                // In paused state, still read frame
                if (hasSeek || quit) {
                    goto mainloop;
                }
                if (tooMuchPackets()) {
                    waitForEvent(10ms);
                    continue;
                }
                if (!readFrame(&eof)) {
                    // Failed to read frame
                    return false;
                }
            }
            if (player->mediaStatus != MediaStatus::BufferingMedia) {
                // Not buffering media, restore it
                doPause(false);
            }
            continue;
        }
        if (player->playbackState == PlaybackState::StoppedState) {
            // If Stopped state, stop the thread and return false.
            quit = true;
        }

        // Check too much packet
        if (tooMuchPackets()) {
            if (waitForEvent(20ms)) {
                continue;
            }
        }
        if (!readFrame(&eof)) {
            // Failed to read frame
            return false;
        }

        // Dosomething like wait if
        if (eof) {
            // Wait here
            if (audioThread) {
                // detect Bug for seeking
                audioThread->packetQueue().put(EofPacket); //< Tell him no more packet
                while (!audioThread->idle() && !quit) {
                    while (waitForEvent(10ms) || hasSeek) {
                        goto mainloop;
                    }
                }
            }
            if (videoThread) {
                videoThread->packetQueue().put(EofPacket); //< Tell him no more packet
                while (!videoThread->idle() && !quit) {
                    while (waitForEvent(10ms) || hasSeek) {
                        goto mainloop;
                    }
                }
            }

            // Time to quit
            if (player->loops == Loops::Infinite) {
                requestSeek(0);
                if (!doSeek()) {
                    return false; //< Seek Error
                }
                goto mainloop;
            }
            quit = true;
        }
    }
    return true;
}
bool DemuxerThread::readFrame(int *eof) {
    isReading = true;
    errcode = av_read_frame(formatCtxt, packet);
    isReading = false;
    if (errcode < 0) {
        if (errcode == AVERROR_EOF) {
            // End of file
            *eof = true;
        }
        else {
            // Error
            return sendError(errcode);
        }
    }
    else {
        // Dispatch here
        if (packet->stream_index == player->audioStream && audioThread) {
            audioThread->packetQueue().put(av_packet_clone(packet));
        }
        else if (packet->stream_index == player->videoStream && videoThread) {
            videoThread->packetQueue().put(av_packet_clone(packet));
        }
        else if (packet->stream_index == player->subtitleStream && subtitleThread) {
            subtitleThread->packetQueue().put(av_packet_clone(packet));
        }
        av_packet_unref(packet);
    }

    if (isLocalSource) {
        return true;
    }

    // Check buffering here
    if (player->mediaStatus == MediaStatus::BufferingMedia) {
        // Buffering progressing
        float curProgress = bufferProgress();
        if ((curProgress - prevBufferProgress) > 0.05f) {
            prevBufferProgress = curProgress;

            // TODO : Do notify the player
            Q_EMIT ffmpegBuffering(bufferedDuration(), curProgress);
        }

        if (hasEnoughPackets() || *eof) {
            // End of buffering
            qDebug() << "DemuxerThread leave buffering";
            Q_EMIT ffmpegBuffering(bufferedDuration(), 1.0);
            player->setMediaStatus(MediaStatus::BufferedMedia);
            if (player->playbackState == PlaybackState::PlayingState) {
                // Restore playing
                doPause(false);
            }
        }
    }
    else if (tooLessPackets() && !(*eof)) {
#if     defined(NEKOAV_BUFFERING_1)
        // Wait for next time 
        if (prevTooLessPacketsTime == 0) {
            // First time not enough data
            prevTooLessPacketsTime = av_gettime_relative();
            return true;
        }
        auto diff = (av_gettime_relative() - prevTooLessPacketsTime) / NEKOAV_TIME_BASE;
        if (diff < 0.02) {
            // If bigger than 20ms, we begin buffering
            return true;
        }
#endif

        prevTooLessPacketsTime = 0; //< Clear the mark
        prevBufferProgress = 0.0f; //< Clear buffering progress
        qDebug() << "DemuxerThread enter buffering";
        player->setMediaStatus(MediaStatus::BufferingMedia);
        doPause(true);
    }
    else {
        prevTooLessPacketsTime = 0; //< Clear the mark
    }
    return true;
}
bool DemuxerThread::tooMuchPackets() const {
    if (audioThread) {
        if (audioThread->packetQueue().size() > bufferedPacketsLimit) {
            return true;
        }
    }
    if (videoThread) {
        if (videoThread->packetQueue().size() > bufferedPacketsLimit) {
            return true;
        }
    }
    return false;
}
bool DemuxerThread::tooLessPackets() const {
    if (audioThread) {
        if (audioThread->packetQueue().size() < bufferedPacketsLessThreshold) {
            return true;
        }
    }
    if (videoThread) {
        if (videoThread->packetQueue().size() < bufferedPacketsLessThreshold) {
            return true;
        }
    }
    return false;
}
bool DemuxerThread::hasEnoughPackets() const {
    if (audioThread) {
        if (audioThread->packetQueue().size() < bufferedPacketsEnough) {
            return false;
        }
    }
    if (videoThread) {
        if (videoThread->packetQueue().size() < bufferedPacketsEnough) {
            return false;
        }
    }
    return true;
}
bool DemuxerThread::sendError(int errc) {
    qDebug() << FFErrorToString(errc);
    Q_EMIT ffmpegErrorOccurred(errc);
    return false;
}
void DemuxerThread::wakeUp() {
    wakeupOnce = true;
    cond.notify_one();
}
bool DemuxerThread::doSeek() {
    // Pause all worker needed
    hasSeek = false;
    
    bool restoreAudio = false;
    bool restoreVideo = false;
    bool restoreSubtitle = false;
    if (audioThread) {
        restoreAudio = !audioThread->isPaused();
        audioThread->pause(true);
        audioThread->packetQueue().flush();
        audioThread->packetQueue().put(FlushPacket);
    }
    if (videoThread && !isPictureStream(player->videoStream)) {
        // We didnot seek if it is a audio cover
        restoreVideo = !videoThread->isPaused();
        videoThread->pause(true);
        videoThread->packetQueue().flush();
        videoThread->packetQueue().put(FlushPacket);
    }
    if (subtitleThread) {
        restoreSubtitle = !subtitleThread->isPaused();
        subtitleThread->pause(true);
        subtitleThread->packetQueue().flush();
        subtitleThread->packetQueue().put(FlushPacket);
        subtitleThread->refresh();
    }

    // Do seek
    int64_t pos = seekPosition * NEKOAV_TIME_BASE;
    errcode = av_seek_frame(formatCtxt, -1, pos, AVSEEK_FLAG_BACKWARD);
    if (errcode < 0) {
        qDebug() << "DemuxerThread failed to seek subtitleStream ";
        return sendError(errcode);
    }

    // Set external Clock 
    externalClock = seekPosition;
    externalClockStart = av_gettime_relative() - externalClock * NEKOAV_TIME_BASE;

    if (restoreAudio) {
        audioThread->pause(false);
    }
    if (restoreVideo) {
        videoThread->pause(false);
    }
    if (restoreSubtitle) {
        subtitleThread->pause(false);
    }

    return true;
}
bool DemuxerThread::waitForEvent(std::chrono::milliseconds ms) {
    doUpdateClock();
    std::unique_lock locker(condMutex);
    return cond.wait_for(locker, ms) == std::cv_status::no_timeout;
}
void DemuxerThread::doUpdateClock() {
    qreal curPos = position();
    if ((curPos - int64_t(curPos)) < 0.1 && abs(curPos - curPosition) >= 1) {
        // Time to update , 1s per second
        curPosition = curPos;

        if (audioThread && videoThread) {
            qDebug() << "Demuxer Position changed to" << curPosition 
                     << " A :" << audioThread->clock()
                     << " V :" << videoThread->clock()
                     << " A-V " << audioThread->clock() - videoThread->clock();  
        }
        else {
            qDebug() << "Demuxer Position changed to" << curPosition;
        }

        // Update position
        Q_EMIT ffmpegPositionChanged(curPosition);
        Q_EMIT ffmpegBuffering(bufferedDuration(), NAN);
    }
    eventDispatcher()->processEvents(QEventLoop::AllEvents);
}
void DemuxerThread::requestSeek(qreal pos) {
    hasSeek = true;
    seekPosition = pos;

    wakeUp();
}
void DemuxerThread::requestSwitchStream(int streamIndex) {
    QMetaObject::invokeMethod(invokeHelper, [this, streamIndex](){
        auto stream = formatCtxt->streams[streamIndex];
        auto type = stream->codecpar->codec_type;

        Q_ASSERT(type == AVMEDIA_TYPE_SUBTITLE);

        // Now try pause it
        bool needRestore = player->playbackState == PlaybackState::PlayingState;
        doPause(true);

        qDebug() << "DemuxerThread::requestSwitchStream switch to " << streamIndex;
        if (type == AVMEDIA_TYPE_SUBTITLE && subtitleThread) {
            // Need change
            player->subtitleStream = streamIndex;
            subtitleThread->switchStream(stream);
        }
        // Seek the stream to the target position
        requestSeek(position());
        doSeek();

        if (needRestore) {
            doPause(false);
        }
    }, Qt::QueuedConnection);
}
void DemuxerThread::doPause(bool v) {
    // Save external clock
    if (v) {
        externalClock = position();
    }
    else {
        // Restore
        externalClockStart = av_gettime_relative() - externalClock * NEKOAV_TIME_BASE;
    }

    // Do work
    if (audioThread) {
        audioThread->pause(v);
    }
    if (videoThread) {
        videoThread->pause(v);
    }
    if (subtitleThread) {
        subtitleThread->pause(v);
    }
}
int DemuxerThread::interruptHandler() {
    // Use this mark to slove pause / playing switching for too slow network connections
    if (wakeupOnce) {
        wakeupOnce = false;

        if (player->playbackState == PlaybackState::PausedState) {
            qDebug() << "DemuxerThread::interruptHandler Pause";
            doPause(true);
        }
        else if (player->playbackState == PlaybackState::PlayingState) {
            if (player->mediaStatus != MediaStatus::BufferingMedia) {
                // not buffering, we can just play
                qDebug() << "DemuxerThread::interruptHandler Play";
                doPause(false);
            }
        }
    }
    return quit;
}
qreal DemuxerThread::clock() const {
    // Has audio, audio master
    if (audioThread) {
        return audioThread->clock();
    }
    // Only video, external master
    if (videoThread) {
        if (videoThread->isPaused()) {
            return externalClock;
        }
    }
    return (av_gettime_relative() - externalClockStart) / NEKOAV_TIME_BASE;
}
qreal DemuxerThread::position() const {
    if (afterSeek) {
        return seekPosition;
    }
    return clock();
}
qreal DemuxerThread::bufferedDuration() const {
    if (!audioThread && !videoThread) {
        return 0.0;
    }
    
    qreal duration = std::numeric_limits<qreal>::max();
    if (audioThread) {
        auto ts = audioThread->packetQueue().duration();
        duration = qMin(ts * av_q2d(formatCtxt->streams[player->audioStream]->time_base), duration);
    }
    if (videoThread && !isPictureStream(player->videoStream)) {
        auto ts = videoThread->packetQueue().duration();
        duration = qMin(ts * av_q2d(formatCtxt->streams[player->videoStream]->time_base), duration);
    }
    return duration;
}
float DemuxerThread::bufferProgress() const {
    if (!audioThread && !videoThread) {
        return 0.0;
    }

    size_t packets = std::numeric_limits<size_t>::max();
    if (audioThread) {
        packets = qMin(packets, audioThread->packetQueue().size());
    }
    if (videoThread && !isPictureStream(player->videoStream)) {
        packets = qMin(packets, videoThread->packetQueue().size());
    }
    if (packets < bufferedPacketsLessThreshold) {
        packets = bufferedPacketsLessThreshold;
    }

    auto progress = double(packets - bufferedPacketsLessThreshold) / double(bufferedPacketsEnough);
    progress = (int64_t(progress * 100)) / 100.0; //< Make it like 0.1 0.2 0.3
    return progress;
}
bool DemuxerThread::isPictureStream(int idx) const {
    // TODO : Improve
    auto stream = formatCtxt->streams[idx];
    return (stream->disposition & AV_DISPOSITION_STILL_IMAGE) || 
           (stream->nb_frames <= 1 && stream->attached_pic.size > 0)
    ;
}

// MediaPlayerPrivate here
MediaPlayerPrivate::~MediaPlayerPrivate() {
    stop();
}
void MediaPlayerPrivate::load() {
    if (demuxerThread) {
        // Alread begin loaded
        return;
    }
    
    // Pending
    QMetaObject::invokeMethod(this, [this]() {
        if (demuxerThread) {
            // Alread begin loaded
            return;
        }
        demuxerThread = new DemuxerThread(this);

        // Prepare connection
        connect(demuxerThread, &DemuxerThread::ffmpegErrorOccurred, 
                this, &MediaPlayerPrivate::demuxerErrorOccurred, 
                Qt::QueuedConnection
        );
        connect(demuxerThread, &DemuxerThread::ffmpegMediaLoaded, 
                this, &MediaPlayerPrivate::demuxerMediaLoaded, 
                Qt::QueuedConnection
        );
        connect(demuxerThread, &DemuxerThread::ffmpegPositionChanged, 
                this, &MediaPlayerPrivate::demuxerPositionChanged, 
                Qt::QueuedConnection
        );
        connect(demuxerThread, &DemuxerThread::ffmpegBuffering, 
                this, &MediaPlayerPrivate::demuxerBuffering,   
                Qt::QueuedConnection
        );

        demuxerThread->start(QThread::HighPriority);
    }, Qt::QueuedConnection);
}
void MediaPlayerPrivate::play() {
    load();

    setPlaybackState(PlaybackState::PlayingState);
}
void MediaPlayerPrivate::stop() {
    if (demuxerThread) {
        delete demuxerThread;
        demuxerThread = nullptr;
    }

    setMediaStatus(MediaStatus::NoMedia);
    setPlaybackState(PlaybackState::StoppedState);
}
void MediaPlayerPrivate::pause() {
    if (!demuxerThread) {
        return;
    }

    setPlaybackState(PlaybackState::PausedState);
}
void MediaPlayerPrivate::setMediaStatus(MediaStatus s) {
    if (s == mediaStatus) {
        return;
    }
    mediaStatus = s;

    auto cb = [this]() {
        Q_EMIT player->mediaStatusChanged(mediaStatus);
    };

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, cb, Qt::QueuedConnection);
    }
    else {
        cb();
    }
}
void MediaPlayerPrivate::setPlaybackState(PlaybackState s) {
    if (s == playbackState) {
        return;
    }
    playbackState = s;

    if (demuxerThread) {
        demuxerThread->wakeUp();
    }

    auto cb = [this]() {
        Q_EMIT player->playingChanged(playbackState == PlaybackState::PlayingState);
        Q_EMIT player->playbackStateChanged(playbackState);
    };

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, cb, Qt::QueuedConnection);
    }
    else {
        cb();
    }
}
void MediaPlayerPrivate::demuxerBuffering(qreal duration, float progress) {
    if (!std::isnan(duration)) {
        Q_EMIT player->bufferedDurationChanged(duration);
    }
    if (!std::isnan(progress)) {
        Q_EMIT player->bufferProgressChanged(progress);
    }
}
void MediaPlayerPrivate::demuxerErrorOccurred(int errcode) {
    errorString = FFErrorToString(errcode);
    error = Error::UnknownError;

    switch (errcode) {
        // TODO 

        // Resource
        case AVERROR_PROTOCOL_NOT_FOUND:
        case AVERROR_INVALIDDATA:
        case AVERROR(ETIMEDOUT): //< Timeout
        case AVERROR(EIO): //< IO ERROR 
            error = Error::ResourceError;
            break;

        case AVERROR_DEMUXER_NOT_FOUND:
        case AVERROR_DECODER_NOT_FOUND:
        case AVERROR_MUXER_NOT_FOUND:
            error = Error::FormatError;
            break;
        
        // Network
        case AVERROR_HTTP_BAD_REQUEST:
        case AVERROR_HTTP_NOT_FOUND:
        case AVERROR_HTTP_OTHER_4XX:
        case AVERROR_HTTP_SERVER_ERROR:
            error = Error::NetworkError;
            break;

        // Network AccessDeniedError
        case AVERROR_HTTP_UNAUTHORIZED:
        case AVERROR_HTTP_FORBIDDEN:
            error = Error::AccessDeniedError;
            break;
    }

    // Release demuxer
    delete demuxerThread;
    demuxerThread = nullptr;

    updateMediaInfo();
    Q_EMIT player->errorChanged();
    Q_EMIT player->errorOccurred(error, errorString);
}
void MediaPlayerPrivate::demuxerMediaLoaded() {
    updateMediaInfo();

    error = Error::NoError;
    Q_EMIT player->errorChanged();
}
void MediaPlayerPrivate::demuxerPositionChanged(qreal pos) {
    Q_EMIT player->positionChanged(pos);
}
void MediaPlayerPrivate::updateMediaInfo() {
    Q_EMIT player->durationChanged(player->duration());
    // Q_EMIT player->positionChanged(player->position());
    Q_EMIT player->seekableChanged(player->isSeekable());
    Q_EMIT player->hasVideoChanged(player->hasVideo());
    Q_EMIT player->hasAudioChanged(player->hasAudio());
    Q_EMIT player->metaDataChanged();
    Q_EMIT player->tracksChanged();
    Q_EMIT player->activeTracksChanged();
}


// Expose public method
MediaPlayer::MediaPlayer(QObject *parent) : QObject(parent), d(new MediaPlayerPrivate(this)) {

}
MediaPlayer::~MediaPlayer() {

}
void MediaPlayer::load() {
    d->load();
}
void MediaPlayer::pause() {
    d->pause();
}
void MediaPlayer::stop() {
    d->stop();
}
void MediaPlayer::play() {
    d->play();
}
void MediaPlayer::setPosition(qreal pos) {
    if (d->demuxerThread) {
        d->demuxerThread->requestSeek(pos);
    }
}
void MediaPlayer::setPlaybackRate(qreal) {

}
auto MediaPlayer::duration() const -> qreal {
    auto ctxt = d->formatContext();
    if (!ctxt) {
        return 0;
    }
    return qreal(ctxt->duration) / AV_TIME_BASE;
}
auto MediaPlayer::playbackState() const -> PlaybackState {
    return d->playbackState;
}
auto MediaPlayer::mediaStatus() const -> MediaStatus {
    return d->mediaStatus;
}
auto MediaPlayer::position() const -> qreal {
    if (!d->demuxerThread) {
        return 0.0;
    }
    return d->demuxerThread->position();
}
auto MediaPlayer::isSeekable() const -> bool {
    auto ctxt = d->formatContext();
    if (!ctxt) {
        return false;
    }
    if (ctxt->pb && strcmp(ctxt->iformat->name, "hls") != 0) {
        if (ctxt->pb->seekable == 0) {
            return false;
        }
    }
    return ctxt->iformat->read_seek || ctxt->iformat->read_seek2;
}
auto MediaPlayer::isPlaying() const -> bool {
    return d->playbackState == PlayingState;
}
auto MediaPlayer::isLoaded() const -> bool {
    return d->demuxerThread && 
           mediaStatus() != NoMedia && 
           mediaStatus() != InvalidMedia && 
           mediaStatus() != LoadingMedia
    ;
}
auto MediaPlayer::hasAudio() const -> bool {
    return d->audioStream >= 0;
}
auto MediaPlayer::hasVideo() const -> bool {
    return d->videoStream >= 0;
}
auto MediaPlayer::loops() const -> int {
    return d->loops;
}
auto MediaPlayer::source() const -> QUrl {
    return d->url;
}
auto MediaPlayer::sourceDevice() const -> const QIODevice * {
    return d->ioDevice;
}
auto MediaPlayer::videoSink() const -> VideoSink * {
    return d->videoSink;
}
auto MediaPlayer::audioOutput() const -> AudioOutput * {
    return d->audioOutput;
}
auto MediaPlayer::bufferedDuration() const -> qreal {
    if (d->demuxerThread) {
        return d->demuxerThread->bufferedDuration();
    }
    return 0.0;
}
auto MediaPlayer::bufferProgress() const -> float {
    if (!d->demuxerThread) {
        return 0.0;
    }
    return d->demuxerThread->bufferProgress();
}
auto MediaPlayer::isAvailable() const -> bool {
    return true;
}
auto MediaPlayer::errorString() const -> QString {
    return d->errorString;
}
auto MediaPlayer::error() const -> Error {
    return d->error;
}

static auto getTracks(AVFormatContext *ctxt, AVMediaType type) -> QList<MediaMetaData> {
    if (!ctxt) {
        return { };
    }
    QList<MediaMetaData> tracks;
    for (int idx = 0; idx < ctxt->nb_streams; idx++) {
        if (ctxt->streams[idx]->codecpar->codec_type != type) {
            continue;
        }
        AVDictionary *metadata = ctxt->streams[idx]->metadata;
        tracks.push_back(MediaMetaData::fromAVDictionary(metadata));
    }

    return tracks;
}
static auto toFFTrack(AVFormatContext *ctxt, int index, AVMediaType type) -> int {
    if (index < 0 || ctxt == nullptr || index >= ctxt->nb_streams) {
        return -1;
    }
    int target = 0;
    for (int idx = 0; idx < ctxt->nb_streams; idx++) {
        if (ctxt->streams[idx]->codecpar->codec_type != type) {
            continue;
        }
        if (target == index) {
            return idx;
        }
        target += 1;
    }
    return target;
}
static auto toNekoTrack(AVFormatContext *ctxt, int index, AVMediaType type) -> int {
    if (index < 0 || ctxt == nullptr || index >= ctxt->nb_streams) {
        return -1;
    }
    int target = 0;
    for (int idx = 0; idx < ctxt->nb_streams; idx++) {
        if (ctxt->streams[idx]->codecpar->codec_type != type) {
            continue;
        }
        if (idx == index) {
            return target;
        }
        target += 1;
    }
    return target;
}

auto MediaPlayer::videoTracks() const -> QList<MediaMetaData> {
    return getTracks(d->formatContext(), AVMEDIA_TYPE_VIDEO);
}
auto MediaPlayer::audioTracks() const -> QList<MediaMetaData> {
    return getTracks(d->formatContext(), AVMEDIA_TYPE_AUDIO);
}
auto MediaPlayer::subtitleTracks() const -> QList<MediaMetaData> {
    return getTracks(d->formatContext(), AVMEDIA_TYPE_SUBTITLE);
}
auto MediaPlayer::metaData() const -> MediaMetaData {
    auto ctxt = d->formatContext();
    if (!ctxt) {
        return { };
    }

    MediaMetaData data;
    AVDictionary *metadata = ctxt->metadata;
    AVDictionaryEntry *cur = av_dict_get(metadata, nullptr, nullptr, 0);
    while (cur) {
        data.insert(cur->key, cur->value);

        cur = av_dict_get(metadata, nullptr, cur, 0);
    }

    return data;
}

auto MediaPlayer::activeAudioTrack() const -> int {
    if (!isLoaded()) {
        return -1;
    }
    return toNekoTrack(d->formatContext(), d->audioStream, AVMEDIA_TYPE_AUDIO);
}
auto MediaPlayer::activeVideoTrack() const -> int {
    if (!isLoaded()) {
        return -1;
    }
    return toNekoTrack(d->formatContext(), d->videoStream, AVMEDIA_TYPE_VIDEO);
}
auto MediaPlayer::activeSubtitleTrack() const -> int {
    if (!isLoaded()) {
        return -1;
    }
    return toNekoTrack(d->formatContext(), d->subtitleStream, AVMEDIA_TYPE_SUBTITLE);
}


// TODO : Runtime changed this track & subtitle support
void MediaPlayer::setActiveAudioTrack(int t)  {
    // Check if loaded
    if (!isLoaded()) {
        return;
    }
    if (t == activeAudioTrack()) {
        return;
    }
    t = toFFTrack(d->formatContext(), t, AVMEDIA_TYPE_AUDIO);
    d->demuxerThread->requestSwitchStream(t);
}
void MediaPlayer::setActiveVideoTrack(int t)  {
    // Check if loaded
    if (!isLoaded()) {
        return;
    }
    if (t == activeVideoTrack()) {
        return;
    }
    t = toFFTrack(d->formatContext(), t, AVMEDIA_TYPE_VIDEO);
    d->demuxerThread->requestSwitchStream(t);
}
void MediaPlayer::setActiveSubtitleTrack(int t)  {
    // Check if loaded
    if (!isLoaded()) {
        return;
    }
    if (t == activeSubtitleTrack()) {
        return;
    }
    t = toFFTrack(d->formatContext(), t, AVMEDIA_TYPE_SUBTITLE);
    d->demuxerThread->requestSwitchStream(t);
}

// Settings
void MediaPlayer::setSource(const QUrl &url) {
    stop();
    d->ioDevice = nullptr;
    d->url = url;
}
void MediaPlayer::setSourceDevice(QIODevice *dev, const QUrl &url) {
    stop();
    d->ioDevice = dev;
    d->url = url;
}
void MediaPlayer::setAudioOutput(AudioOutput *output) {
    if (d->audioOutput) {
        d->audioOutput->disconnect(d.get());
    }
    if (output) {
        connect(output, &AudioOutput::aboutToDestroy, d.get(), [this]() {
            d->stop();
            d->audioOutput = nullptr;

            Q_EMIT audioOutputChanged();
        });
    }

    d->audioOutput = output;
    Q_EMIT audioOutputChanged();
}
void MediaPlayer::setVideoSink(VideoSink *sink) {
    if (d->videoSink) {
        d->videoSink->disconnect(d.get());
    }
    if (sink) {
        connect(sink, &VideoSink::aboutToDestroy, d.get(), [this]() {
            d->stop();
            d->videoSink = nullptr;

            Q_EMIT videoOutputChanged();
        });
    }

    d->videoSink = sink;
    Q_EMIT videoOutputChanged();
}
void MediaPlayer::setVideoOutput(GraphicsVideoItem *item) {
    setVideoSink(item->videoSink());
}
void MediaPlayer::setVideoOutput(VideoWidget *item) {
    setVideoSink(item->videoSink());
}
void MediaPlayer::setOption(const QString &key, const QString &value) {
    av_dict_set(&d->options, key.toUtf8().data(), value.toUtf8().data(), 0);
}
void MediaPlayer::setHttpUseragent(const QString &useragent) {
    av_dict_set(&d->options, "user_agent", useragent.toUtf8().data(), 0);
}
void MediaPlayer::setHttpReferer(const QString &referer) {
    av_dict_set(&d->options, "referer", referer.toUtf8().data(), 0);
}
void MediaPlayer::setInputFormat(AVInputFormat *i) {
    d->inputFormat = i;
}
void MediaPlayer::clearOptions() {
    av_dict_free(&d->options);
}
QStringList MediaPlayer::supportedMediaTypes() {
    QStringList types;

    void *op = nullptr;
    auto i = av_demuxer_iterate(&op);
    while (i) {
        types.push_back(i->name);

        i = av_demuxer_iterate(&op);
    }
    return types;
}
QStringList MediaPlayer::supportedProtocols() {
    QStringList protocols;

    void *op = nullptr;
    auto p = avio_enum_protocols(&op, 0);
    while (p) {
        protocols.push_back(p);

        p = avio_enum_protocols(&op, 0);
    }

    return protocols;
}


// Video Sink
VideoSink::VideoSink(QObject *parent) : QObject(parent) {
    addPixelFormat(VideoPixelFormat::RGBA32);
}
VideoSink::~VideoSink() {
    Q_EMIT aboutToDestroy();
}
void  VideoSink::setVideoFrame(const VideoFrame &f) {
    frame = f;
    if (frame.isNull()) {
        *mark = false;
        mark = std::make_shared<bool>(false);
    }
    if (size.width() != frame.width() || size.height() != frame.height()) {
        size.setWidth(frame.width());
        size.setHeight(frame.height());


        QMetaObject::invokeMethod(this, [this]() {
            Q_EMIT videoSizeChanged();
        }, Qt::QueuedConnection);
    }
    QMetaObject::invokeMethod(this, [this, m = mark]() {
        if (!m) {
            qDebug() << "VideoSink::putVideoFrame cancelled call";
            return;
        }
        Q_EMIT videoFrameChanged(frame);
    }, Qt::QueuedConnection);
}
void  VideoSink::setSubtitleText(const QString &text) {
    subtitle = text;
    QMetaObject::invokeMethod(this, [this, text]() {
        Q_EMIT subtitleTextChanged(text);
    }, Qt::QueuedConnection);
}
void  VideoSink::addPixelFormat(VideoPixelFormat fmt) {
    formats.push_back(fmt);
}
QSize VideoSink::videoSize() const {
    return size;
}
VideoFrame VideoSink::videoFrame() const {
    return frame;
}
QString VideoSink::subtitleText() const {
    return subtitle;
}
QList<VideoPixelFormat> VideoSink::supportedPixelFormats() const {
    return formats;
}


// Video Frame
// May memory leak ?
VideoFrame::VideoFrame() {

}
VideoFrame::VideoFrame(const VideoFrame &b) = default;
VideoFrame::VideoFrame(VideoFrame &&) = default;
VideoFrame::~VideoFrame() {
    // av_frame_free(reinterpret_cast<AVFrame**>(&d));
}
int VideoFrame::width() const {
    if (isNull()) {
        return 0;
    }
    return d->frame->width;
}
int VideoFrame::height() const {
    if (isNull()) {
        return 0;
    }
    return d->frame->height;
}
QSize VideoFrame::size() const {
    return QSize(width(), height());
}
bool VideoFrame::isNull() const {
    return d == nullptr;
}
int  VideoFrame::planeCount() const {
    if (isNull()) {
        return 0;
    }
    return av_pix_fmt_count_planes(AVPixelFormat(d->frame->format));
}
void VideoFrame::lock() const {
    // if (d) {
    //     if (d->opaque) {
    //         static_cast<std::mutex*>(d->opaque)->lock();
    //     }
    // }
    if (!isNull()) {
        // d->mutex.lock();
    }
}
void VideoFrame::unlock() const {
    // if (d) {
    //     if (d->opaque) {
    //         static_cast<std::mutex*>(d->opaque)->unlock();
    //     }
    // }
    if (!isNull()) {
        // d->mutex.unlock();
    }
}
uchar *VideoFrame::bits(int plane) const {
    if (isNull()) {
        return nullptr;
    }
    Q_ASSERT(plane < planeCount());
    return d->frame->data[plane];
}
int    VideoFrame::bytesPerLine(int plane) const {
    if (isNull()) {
        return 0;
    }
    Q_ASSERT(plane < planeCount());
    return d->frame->linesize[plane];
}
QImage VideoFrame::toImage() const {
    QImage image(width(), height(), QImage::Format_RGBA8888);

    int w = width();
    int h = height();
    int pitch = bytesPerLine(0);
    uchar *pixels = bits(0);

    uchar *dst = image.bits();
    int    dstPitch = image.bytesPerLine();

    // Update it
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            *((uint32_t*)   &dst[y * dstPitch + x * 4]) = *(
                (uint32_t*) &pixels[y * pitch + x * 4]
            );
        }
    }

    return image;
}
VideoPixelFormat VideoFrame::pixelFormat() const {
    if (isNull()) {
        return VideoPixelFormat::Invalid;
    }
    return ToVideoPixelFormat(AVPixelFormat(d->frame->format));
}

VideoFrame VideoFrame::fromAVFrame(AVFrame *avframe) {
    VideoFrame f;
    // if (avframe) {
    //     f.d = static_cast<VideoFramePrivate*>(av_frame_clone(static_cast<AVFrame*>(avframe)));
    // }
    if (avframe) {
        auto copy = av_frame_clone(static_cast<AVFrame*>(avframe));
        f.d = QSharedPointer<VideoFramePrivate>::create(copy);
    }
    return f;
}
// VideoFrame &VideoFrame::operator =(const VideoFrame &other) {
//     if (&other == this) {
//         return *this;
//     }
//     // av_frame_free(reinterpret_cast<AVFrame**>(&d));
//     // if (other.d) {
//     //     d = static_cast<VideoFramePrivate*>(av_frame_clone(other.d));
//     // }
//     d = other.d;

//     return *this;
// }
VideoFrame &VideoFrame::operator =(const VideoFrame &other) = default;
VideoFrame &VideoFrame::operator =(VideoFrame &&other) = default;

// Dictionary
void Dictionary::load(const AVDictionary *dict) {
    AVDictionaryEntry *cur = av_dict_get(dict, "", nullptr, AV_DICT_IGNORE_SUFFIX);
    while (cur) {
        insert(cur->key, cur->value);

        cur = av_dict_get(dict, "", cur, AV_DICT_IGNORE_SUFFIX);
    }
}
AVDictionary *Dictionary::toAVDictionary() const {
    AVDictionary *dict = nullptr;
    for (const auto &key : keys()) {
        av_dict_set(&dict, key.toUtf8().constData(), value(key).toUtf8().constData(), 0);
    }
    return dict;
}

}
