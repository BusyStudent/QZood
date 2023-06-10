#define NEKO_SOURCE
#include "nekoav.hpp"
#include "nekoprivate.hpp"
#include <QAbstractEventDispatcher>
#include <QIODevice>

namespace NekoAV {

// Packet Queue Part
PacketQueue::PacketQueue() = default;
PacketQueue::~PacketQueue() {
    flush();
}

void PacketQueue::put(AVPacket *packet) {
    std::lock_guard locker(mutex);
    packets.push(packet);
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
        packets.pop();
    }
    packetsDuration = 0;
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
        if (!block) {
            mutex.unlock();
            return nullptr;
        }

        mutex.unlock();

        qDebug() << "PacketQueue waiting for more packets...";

        std::unique_lock lock(condMutex);
        cond.wait(lock);

        mutex.lock();
    }
    ret = packets.front();
    packets.pop();
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
    audioOutput->setCallback([this](void *data, int n) {
        audioCallback(data, n);
    });   
    audioOutput->open(
        AudioSampleFormat::Float32,
        ctxt->sample_rate,
        ctxt->channels
    );
    audioOutput->pause(true);
}
AudioThread::~AudioThread() {
    audioOutput->close();

    avcodec_free_context(&codecCtxt);
}
void AudioThread::pause(bool v) {
    audioOutput->pause(v);
}
void AudioThread::audioCallback(void *data, int len) {
    uint8_t *dst = static_cast<uint8_t*>(data);
    while (len > 0) {
        if (bufferIndex >= buffer.size()) {
            // Run out of buffer
            int audioSize = audioDecodeFrame();
            if (audioSize < 0) {
                // No audio
                break;
            }
        }

        // Write buffer
        int left = buffer.size() - bufferIndex;
        left = qMin(len, left);

        ::memcpy(dst, buffer.data() + bufferIndex, left);
        
        dst += left;
        
        len -= left;
        bufferIndex += left;
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
        if (packet == StopPacket) {
            waitting = true;
            return -1;
        }
        if (packet == nullptr) {
            // No Data
            waitting = true;
            return -1;
        }
        waitting = false;

        // Normal data
        AVPtr<AVPacket> guard(packet);

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
        buffer.resize(out_size);
        buffer_data = buffer.data();

        int len2 = swr_convert(swrCtxt.get(), out, out_count, in, frame->nb_samples);
        if (len2 < 0) {
            qDebug() << ("swr_convert failed\n");
            return -1;
        }
        if (len2 == out_count) {
            // BTK_LOG("audio buffer is probably too small\n");
        }
        int resampled_data_size = len2 * codecCtxt->channels * av_get_bytes_per_sample(codecCtxt->sample_fmt);
        buffer.resize(resampled_data_size);
        bufferIndex = 0;
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
    QThread(),
    demuxerThread(parent),
    stream(stream),
    codecCtxt(ctxt)    
{
    setObjectName("NekoAV VideoThread");
    
    videoSink = demuxerThread->videoSink();

    // Prepare convertion buffer
    size_t n     = av_image_get_buffer_size(AV_PIX_FMT_RGBA, ctxt->width, ctxt->height, 32);
    uint8_t *buf = static_cast<uint8_t*>(av_malloc(n));

    av_image_fill_arrays(
        dstFrame->data,
        dstFrame->linesize,
        buf,
        AV_PIX_FMT_RGBA,
        ctxt->width, ctxt->height,
        32
    );

    start();
    pause(true);
}
VideoThread::~VideoThread() {
    queue.flush();
    queue.put(StopPacket);
    pause(false);
    // Wait
    wait();

    avcodec_free_context(&codecCtxt);
}
void VideoThread::run() {
    videoClockStart = av_gettime_relative();
    swsScaleDuration = 0.0;
    // Try get packet
    int ret;
    while (true) {
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
        if (packet == StopPacket) {
            waitting = true;
            break; //< It tell us, it time to stop
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
        double masterClock = demuxerThread->position();
        double diff = masterClock - videoClock - swsScaleDuration;

        videoClock = currentFramePts;

        if (diff < 0 && -diff < AVNoSyncThreshold) {
            // We are too fast
            auto delay = -diff;

            std::unique_lock lock(condMutex);
            cond.wait_for(lock, std::chrono::milliseconds(int64_t(delay * 1000)));
        }
        else if (diff > 0.3) {
            // We are too slow, drop
            // BTK_LOG(BTK_RED("[VideoThread] ") "A-V = %lf Too slow, drop sws_duration = %lf\n", diff, sws_scale_duration);
            continue;
        }
        else if (diff < AVSyncThreshold){
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
    videoSink->putVideoFrame(VideoFrame());
}
bool VideoThread::videoDecodeFrame(AVPacket *packet, AVFrame **retFrame) {
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
    return true;
}
void VideoThread::videoWriteFrame(AVFrame *source) {
    
    // Lazy eval beacuse of the hardware access
    if (!swsCtxt) {
        swsCtxt.reset(
            sws_getContext(
                codecCtxt->width,
                codecCtxt->height,
                AVPixelFormat(source->format),
                codecCtxt->width,
                codecCtxt->height,
                AV_PIX_FMT_RGBA,
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
        dstFrame->format = AV_PIX_FMT_RGBA;
        dstFrame->opaque = &dstFrameMutex;
    }

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

    swsScaleDuration = (av_gettime_relative() - swsBeginTime) / 1000000.0;

    if (ret < 0) {
        // BTK_LOG(BTK_RED("[VideoThread] ") "sws_scale failed %d!!!\n", ret);
        return;
    }

    videoSink->putVideoFrame(VideoFrame::fromAVFrame(dstFrame.get()));
}
void VideoThread::pause(bool v) {
    if (paused == v) {
        return;
    }
    paused = v;
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
    if (!load()) {
        return;
    }
    if (prepareWorker()) {
        runDemuxer();
    }
    
    // Settings some status
    Q_EMIT ffmpegPositionChanged(0.0);
    player->setMediaStatus(MediaStatus::NoMedia);
    player->setPlaybackState(PlaybackState::StoppedState);

    // Cleanup
    delete audioThread;
    delete videoThread;

    audioThread = nullptr;
    videoThread = nullptr;

    avformat_close_input(&formatCtxt);
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
        auto self = static_cast<DemuxerThread *>(_self);
        
        // Distach event if
        self->eventDispatcher()->processEvents(QEventLoop::AllEvents);
        return self->quit;
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
    auto stdstr = player->url.toUtf8();

    player->setMediaStatus(MediaStatus::LoadingMedia);
    errcode = avformat_open_input(
        &formatCtxt,
        stdstr.data(),
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
    av_dump_format(formatCtxt, 0, stdstr.data(), 0);

    player->setMediaStatus(MediaStatus::LoadedMedia);
    player->loaded = true;

    // Get Stream of it
    player->videoStream = av_find_best_stream(formatCtxt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    player->audioStream = av_find_best_stream(formatCtxt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

    if (player->audioStream < 0 && player->videoStream < 0) {
        // No stream !!!
        errcode = AVERROR_STREAM_NOT_FOUND;
        return sendError(errcode);
    }

    Q_EMIT ffmpegMediaLoaded();

    // Check the URL if is network stream
    if (stdstr.startsWith("http")) {
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
    return true;
}
bool DemuxerThread::prepareCodec(int streamid) {
    auto stream = formatCtxt->streams[streamid];
    auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        // No codec
        errcode = AVERROR_DECODER_NOT_FOUND;
        return sendError(errcode);
    }
    auto codecCtxt = avcodec_alloc_context3(codec);
    if (!codec) {
        errcode = AVERROR(ENOMEM);
        return sendError(errcode);
    }
    errcode = avcodec_parameters_to_context(codecCtxt, stream->codecpar);
    if (errcode < 0) {
        avcodec_free_context(&codecCtxt);
        return sendError(errcode);
    }
    errcode = avcodec_open2(codecCtxt, codecCtxt->codec, nullptr);
    if (errcode < 0) {
        avcodec_free_context(&codecCtxt);
        return sendError(errcode);
    }

    // Common init done
    if (codecCtxt->codec_type == AVMEDIA_TYPE_AUDIO) {
        audioThread = new AudioThread(this, stream, codecCtxt);
        return true;
    }
    else if (codecCtxt->codec_type == AVMEDIA_TYPE_VIDEO) {
        videoThread = new VideoThread(this, stream, codecCtxt);
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
        // if (player->playbackState == PlaybackState::PlayingState) {
        //     // If Stopped state, stop the thread and return false.
        //     if (audioThread) {
        //         audioThread->pause(false);
        //     }
        //     if (videoThread) {
        //         videoThread->pause(false);
        //     }
        // }

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
    errcode = av_read_frame(formatCtxt, packet);
    if (errcode < 0) {
        if (errcode == AVERROR_EOF) {
            // 
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
        av_packet_unref(packet);
    }

    if (isLocalSource) {
        return true;
    }

    // Check buffering here
    if (player->mediaStatus == MediaStatus::BufferingMedia) {
        if (hasEnoughPackets() || *eof) {
            // End of buffering
            qDebug() << "DemuxerThread leave buffering";
            player->setMediaStatus(MediaStatus::BufferedMedia);
            if (player->playbackState == PlaybackState::PlayingState) {
                // Restore playing
                doPause(false);
            }
        }
        else {
            // Buffering progressing
            float curProgress = bufferProgress();
            if ((curProgress - prevBufferProgress) > 0.1) {
                prevBufferProgress = curProgress;

                // TODO : Do notify the player
                Q_EMIT ffmpegBufferProgressChanged(curProgress);
            }
        }
    }
    else {
        if (tooLessPackets()) {
            prevBufferProgress = 0.0f; //< Clear buffering progress
            qDebug() << "DemuxerThread enter buffering";
            player->setMediaStatus(MediaStatus::BufferingMedia);
            doPause(true);
        }
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
    cond.notify_one();
}
bool DemuxerThread::doSeek() {
    // Pause all worker needed
    hasSeek = false;
    
    bool restoreAudio = false;
    bool restoreVideo = false;
    if (audioThread) {
        restoreAudio = !audioThread->isPaused();
        audioThread->pause(true);
        audioThread->packetQueue().flush();
        audioThread->packetQueue().put(FlushPacket);

        // Do seek
        int64_t pos = seekPosition / av_q2d(formatCtxt->streams[player->audioStream]->time_base);
        errcode = av_seek_frame(formatCtxt, player->audioStream, pos, AVSEEK_FLAG_BACKWARD);
        if (errcode < 0) {
            qDebug() << "DemuxerThread failed to seek audioStream ";
            return sendError(errcode);
        }
    }
    if (videoThread) {
        // We didnot seek if it is a audio cover
        restoreVideo = !videoThread->isPaused();
        videoThread->pause(true);
        videoThread->packetQueue().flush();
        videoThread->packetQueue().put(FlushPacket);

        int64_t pos = seekPosition / av_q2d(formatCtxt->streams[player->videoStream]->time_base);
        errcode = av_seek_frame(formatCtxt, player->videoStream, pos, AVSEEK_FLAG_BACKWARD);
        if (errcode < 0) {
            qDebug() << "DemuxerThread failed to seek videoStream ";
            return sendError(errcode);
        }
    }

    if (restoreAudio) {
        audioThread->pause(false);
    }
    if (restoreVideo) {
        videoThread->pause(false);
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
    if (abs(curPos - curPosition) >= 1) {
        // Time to update
        curPosition = curPos;

        if (audioThread && videoThread) {
            qDebug() << "Demuxer Position changed to" << curPosition << " A-V " << audioThread->clock() - videoThread->clock();  
        }
        else {
            qDebug() << "Demuxer Position changed to" << curPosition;
        }

        // Update position
        Q_EMIT ffmpegPositionChanged(curPosition);
    }
}
void DemuxerThread::requestSeek(qreal pos) {
    hasSeek = true;
    seekPosition = pos;

    wakeUp();
}
void DemuxerThread::doPause(bool v) {
    if (audioThread) {
        audioThread->pause(v);
    }
    if (videoThread) {
        videoThread->pause(v);
    }
}
qreal DemuxerThread::position() const {
    if (audioThread) {
        return audioThread->clock();
    }
    qDebug() << "DemuxerThread::position warning no audioThread";
    return 0.0;
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
    if (videoThread) {
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
    if (videoThread) {
        packets = qMin(packets, videoThread->packetQueue().size());
    }

    return double(packets - bufferedPacketsLessThreshold) / double(bufferedPacketsEnough);
}
bool DemuxerThread::isPictureStream(int idx) const {
    // TODO : Improve
    auto stream = formatCtxt->streams[idx];
    return (stream->disposition & AV_DISPOSITION_STILL_IMAGE) || 
           (stream->nb_frames <= 1 && stream->attached_pic.data) ||
           (stream->codecpar->codec_id = AV_CODEC_ID_JPEG2000) ||
           (stream->codecpar->codec_id = AV_CODEC_ID_PNG)
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
        connect(demuxerThread, &DemuxerThread::ffmpegBufferProgressChanged, 
                this, &MediaPlayerPrivate::demuxerBufferProgressChanged,  
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
void MediaPlayerPrivate::demuxerBufferProgressChanged(float progress) {
    Q_EMIT player->bufferProgressChanged(progress);
}
void MediaPlayerPrivate::demuxerErrorOccurred(int errcode) {
    errorString = FFErrorToString(errcode);
    error = Error::UnknownError;

    switch (errcode) {
        // TODO 

        // Resource
        case AVERROR_PROTOCOL_NOT_FOUND:
        case AVERROR_INVALIDDATA: 
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
    Q_EMIT player->activeTracksChanged();
    Q_EMIT player->metaDataChanged();
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
    if (ctxt->pb) {
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
        MediaMetaData data;
        AVDictionary *metadata = ctxt->streams[idx]->metadata;
        AVDictionaryEntry *cur = av_dict_get(metadata, nullptr, nullptr, 0);
        while (cur) {
            data.insert(cur->key, cur->value);

            cur = av_dict_get(metadata, nullptr, cur, 0);
        }

        tracks.push_back(data);
    }

    return tracks;
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
    return d->audioStream;
}
auto MediaPlayer::activeVideoTrack() const -> int {
    if (!isLoaded()) {
        return -1;
    }
    return d->videoStream;
}
auto MediaPlayer::activeSubtitleTrack() const -> int {
    return -1;
}


// TODO : Runtime changed this track & subtitle support
void MediaPlayer::setActiveAudioTrack(int t)  {
    // Check if loaded
    if (!isLoaded() || isPlaying()) {
        return;
    }
    d->audioStream = t;
}
void MediaPlayer::setActiveVideoTrack(int t)  {
    // Check if loaded
    if (!isLoaded() || isPlaying()) {
        return;
    }
    d->videoStream = t;
}
void MediaPlayer::setActiveSubtitleTrack(int t)  {
    // Check if loaded
    if (!isLoaded() || isPlaying()) {
        return;
    }
}

// Settings
void MediaPlayer::setSource(const QUrl &url) {
    stop();
    d->ioDevice = nullptr;
    if (url.isLocalFile()) {
        d->url = url.toLocalFile();
    }
    else {
        d->url = url.toString();
    }
}
void MediaPlayer::setSourceDevice(QIODevice *dev, const QUrl &url) {
    stop();
    d->ioDevice = dev;
    d->url = url.toLocalFile();
}
void MediaPlayer::setAudioOutput(AudioOutput *output) {
    d->audioOutput = output;
    Q_EMIT audioOutputChanged();
}
void MediaPlayer::setVideoSink(VideoSink *sink) {
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
void MediaPlayer::setInputFormat(void *i) {
    d->inputFormat = static_cast<AVInputFormat*>(i);
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
    
}
VideoSink::~VideoSink() {

}
void  VideoSink::putVideoFrame(const VideoFrame &f) {
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
QSize VideoSink::videoSize() const {
    return size;
}

// Video Frame
int VideoFrame::width() const {
    if (isNull()) {
        return 0;
    }
    return d->width;
}
int VideoFrame::height() const {
    if (isNull()) {
        return 0;
    }
    return d->height;
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
    return av_pix_fmt_count_planes(AVPixelFormat(d->format));
}
void VideoFrame::lock() const {
    if (d) {
        if (d->opaque) {
            static_cast<std::mutex*>(d->opaque)->lock();
        }
    }
}
void VideoFrame::unlock() const {
    if (d) {
        if (d->opaque) {
            static_cast<std::mutex*>(d->opaque)->unlock();
        }
    }
}
uchar *VideoFrame::bits(int plane) const {
    if (isNull()) {
        return nullptr;
    }
    // Q_ASSERT(plane < planeCount());
    return d->data[plane];
}
int    VideoFrame::bytesPerLine(int plane) const {
    if (isNull()) {
        return 0;
    }
    // Q_ASSERT(plane < planeCount());
    return d->linesize[plane];
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
    switch (d->format) {
        case AV_PIX_FMT_RGBA : return VideoPixelFormat::RGBA32;
        default :              return VideoPixelFormat::Invalid;
    }
}

VideoFrame VideoFrame::fromAVFrame(void *avframe) {
    VideoFrame f;
    f.d = static_cast<VideoFramePrivate*>(avframe);
    return f;
}

}
