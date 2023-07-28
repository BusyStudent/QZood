#include "videocanvas.hpp"
#include "videocanvasprivate.hpp"

#include "../common/myGlobalLog.hpp"
#include <QTimerEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLPaintDevice>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QFontMetricsF>
#include <QTextCursor>
#include <QPainter>
#include <mutex>

// OpenGL parts
#if !defined(NDEBUG)
namespace {
    #define VGL_CHECK_ERROR() _vglCheckError(gl.get(), __FUNCTION__, __LINE__)
    void _vglCheckError(QOpenGLFunctions_3_3_Core *fn, const char *file, int line) {
        auto e = fn->glGetError();
        switch (e) {
            case GL_NO_ERROR: return;
            case GL_INVALID_OPERATION: qCritical() << "GL_INVALID_OPERATION at" << file << ":" << line; break;
            case GL_INVALID_VALUE: qCritical() << "GL_INVALID_VALUE at" << file << ":" << line; break;
            case GL_INVALID_ENUM: qCritical() << "GL_INVALID_ENUM at" << file << ":" << line; break;
        }
        qCritical() << (char*)fn->glGetString(GL_VERSION);
    }
}
#else
    #define VGL_CHECK_ERROR() 
#endif
// #define QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL


VideoCanvas::VideoCanvas(QWidget *parent) : QOpenGLWidget(parent), d(new VideoCanvasPrivate(this)) {

}
VideoCanvas::~VideoCanvas() {
    makeCurrent();

#if !defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    d->cleanupGL();
    d.reset();
#endif
}
void VideoCanvas::attachPlayer(NekoMediaPlayer *player) {
    d->player = player;

    player->setVideoSink(&d->videoSink);

    connect(player, &NekoMediaPlayer::playbackStateChanged, d.get(), &VideoCanvasPrivate::_on_playerStateChanged);
}
void VideoCanvas::setDanmakuList(const DanmakuList &list) {
    Q_ASSERT(d->player);
    d->clearTracks();
    d->danmakuList = list;
    d->danmakuIter = d->danmakuList.cbegin();

    if (d->danmakuTimer == 0 && d->player->playbackState() == NekoMediaPlayer::PlayingState) {
        // Start timer
        d->danmakuTimer = d->startTimer(1000 / d->danmakuFps, Qt::PreciseTimer);
    }
    update();
}
void VideoCanvas::setDanmakuPosition(qreal position) {
    d->clearTracks();
    if (d->danmakuList.empty()) {
        return;
    }

    d->danmakuIter = d->danmakuList.begin();
    while (d->danmakuIter->position < position && d->danmakuIter != d->danmakuList.cend()) {
        ++(d->danmakuIter);
    }
    if (d->danmakuIter != d->danmakuList.cend()) {
        qDebug() << "Seek to pos: " << d->danmakuIter->position << " text: " << d->danmakuIter->text;
    }
    update();
}
void VideoCanvas::setDanmakuTracksLimit(qreal limit) {
    d->danmakuTracksLimit = std::clamp(limit, 0.0, 1.0);
    d->resizeTracks();
}
void VideoCanvas::setDanmakuAliveTime(qreal t) {
    d->danmakuAliveTime = t;
    update();
}
void VideoCanvas::setDanmakuShadowMode(ShadowMode m) {

}
void VideoCanvas::setDanmakuVisible(bool visible) {
    d->danmakuVisible = visible;
}
void VideoCanvas::setDanmakuOpacity(qreal op) {
    d->danmakuOpacity = op;
    update();    
}
void VideoCanvas::setDanmakuFont(const QFont &font) {
    d->danmakuFont = font;
    update();
}

void VideoCanvas::setAspectMode(AspectMode mode) {
    d->aspectMode = mode;

#if !defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    // We need update GL
    if (d->textures[0]) {
        makeCurrent();
        d->updateGLBuffer();
    }
#endif

    update();
}
void VideoCanvas::setSubtitleFont(const QFont &f) {
    d->subtitleFont = f;
    update();
}
void VideoCanvas::setSubtitleOpacity(qreal op) {
    d->subtitleOpacity = op;
    update();
}
void VideoCanvas::setSubtitleColor(const QColor &c) {
    d->subtitleColor = c;
    update();
}
void VideoCanvas::setSubtitleOutlineColor(const QColor &c) {
    d->subtitleOutlineColor = c;
    update();
}

qreal VideoCanvas::danmakuTracksLimit() const {
    return d->danmakuTracksLimit;
}
qreal VideoCanvas::danmakuOpacity() const {
    return d->danmakuOpacity;
}
QFont VideoCanvas::danmakuFont() const {
    return d->danmakuFont;
}
QFont VideoCanvas::subtitleFont() const {
    return d->subtitleFont;
}
qreal VideoCanvas::subtitleOpacity() const {
    return d->subtitleOpacity;
}
QColor VideoCanvas::subtitleColor() const {
    return d->subtitleColor;
}
QColor VideoCanvas::subtitleOutlineColor() const {
    return d->subtitleOutlineColor;
}
auto   VideoCanvas::aspectMode() const -> AspectMode {
    return d->aspectMode;
}
auto   VideoCanvas::danmakuShadowMode() const -> ShadowMode {
    return d->danmakuShadowMode;
}

void VideoCanvas::paintGL() {
    QPainter painter(this);

#if defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    painter.fillRect(0, 0, width(), height(), Qt::black);
#else
    painter.beginNativePainting();
    d->paintGL();
    painter.endNativePainting();
#endif

    d->paint(painter);
}
void VideoCanvas::resizeEvent(QResizeEvent *e) {
    QOpenGLWidget::resizeEvent(e);
    d->resizeTracks();
}
void VideoCanvas::resizeGL(int w, int h) {
    auto fns = QOpenGLContext::currentContext()->functions();

    fns->glViewport(0, 0, w, h);

#if !defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    d->resizeGL(w, h);
#endif
}
void VideoCanvas::initializeGL() {
#if !defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    auto ctxt = QOpenGLContext::currentContext();
    d->gl.reset(new QOpenGLFunctions_3_3_Core);
    d->gl->initializeOpenGLFunctions();
    d->initializeGL();

    // Set GL cleanup function
    QObject::connect(ctxt, &QOpenGLContext::aboutToBeDestroyed, d.get(), &VideoCanvasPrivate::cleanupGL);
#endif
}

VideoCanvasPrivate::VideoCanvasPrivate(VideoCanvas *parent) : QObject(parent), videoCanvas(parent) {
    connect(&videoSink, &NekoVideoSink::videoFrameChanged, this, &VideoCanvasPrivate::_on_VideoFrameChanged, Qt::QueuedConnection);
    connect(&videoSink, &NekoVideoSink::subtitleTextChanged, this, &VideoCanvasPrivate::_on_SubtitleTextChanged, Qt::QueuedConnection);

#if !defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    videoSink.addPixelFormat(NekoVideoPixelFormat::YUV420P);
    videoSink.addPixelFormat(NekoVideoPixelFormat::NV12);
#endif
}
void VideoCanvasPrivate::paint(QPainter &painter) {
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::Antialiasing, true);

#if defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    if (!image.isNull()) {
        painter.drawImage(viewportRect(), image);
    }
#endif
    if (player) {
        painter.setFont(videoCanvas->font());
        painter.setPen(Qt::white);
        
        if (danmakuIter != danmakuList.cend()) {
            painter.drawText(0, 0, videoCanvas->width(), videoCanvas->height(), Qt::AlignBottom,
                QString::asprintf("Tracks %d Progress %.1lf to %.1lf buffered %.1lf Danmaku %.1lf %s", 
                    int(danmakuTracks.size()), player->position(), player->duration(), player->bufferedDuration(),
                    danmakuIter->position, danmakuIter->text.toUtf8().constData()
                )
            );
        }
        else {
            painter.drawText(0, 0, videoCanvas->width(), videoCanvas->height(), Qt::AlignBottom,
                QString::asprintf("Tracks %d Progress %.1lf to %.1lf buffered %.1lf", 
                    int(danmakuTracks.size()), player->position(), player->duration(), player->bufferedDuration()
                )
            );
        }
    }

    // Then paint danmaku
    paintDanmaku(painter);

    // Paint the subtitles
    if (hasSubtitle) {
        // Calc position
        auto textSize = subtitleText.size();

        qreal x = videoCanvas->width() / 2.0 - textSize.width() / 2.0;
        qreal y = videoCanvas->height() - textSize.height();

        painter.save();
        painter.setOpacity(subtitleOpacity);
        painter.setFont(subtitleFont);

        painter.setPen(subtitleOutlineColor);
        painter.drawStaticText(x + 1, y + 1, subtitleText);

        painter.setPen(subtitleColor);
        painter.drawStaticText(x, y, subtitleText);
        // painter.setPen(subtitleOutlineColor);
        // painter.drawPath(subtitlePath);
        painter.restore();
    }
}
void VideoCanvasPrivate::paintDanmaku(QPainter &painter) {
    if (!danmakuVisible) {
        return;
    }

    painter.save();
    painter.setOpacity(painter.opacity() * danmakuOpacity);
    for (auto &tracks : danmakuTracks) {
        for (auto &node : tracks) {
            painter.setFont(node.font);

            painter.setPen(Qt::darkGray);
            painter.drawStaticText(node.x + 1, node.y + 1, node.text);

            painter.setPen(node.data->color);
            painter.drawStaticText(node.x, node.y, node.text);
        }
    }

    qreal screenWidth = videoCanvas->width();
    for (auto &node : danmakuTopBottomTrack) {
        qreal x = (screenWidth / 2) - (node.w / 2);
        qreal y = node.y;

        painter.setFont(node.font);

        painter.setPen(Qt::darkGray);
        painter.drawStaticText(x + 1, y + 1, node.text);

        painter.setPen(node.data->color);
        painter.drawStaticText(x, y, node.text);
    }

    painter.restore();
}
void VideoCanvasPrivate::resizeTracks() {
    qreal height = videoCanvas->height();
    int num = height / (DanmakuItem::Medium * danmakuScale + danmakuSpacing);
    num *= danmakuTracksLimit;
    danmakuTracks.resize(num);
}
void VideoCanvasPrivate::timerEvent(QTimerEvent *event) {
    if (event->timerId() != danmakuTimer) {
        return;
    }
    qreal position = player->position();
    qreal width = videoCanvas->width();

    qreal diff = 0;
    if (danmakuIter != danmakuList.cend()) {
        diff = position - danmakuIter->position;
    }

    // BTK_LOG("cur pos %lf\n", pos);
    if (danmakuIter != danmakuList.cend() && std::abs(diff) < 5) {
        if (std::abs(diff) > 10) {
            // Too big
            qWarning() << diff;
        }

        while (danmakuIter->position < position && danmakuIter != danmakuList.cend()) {
            // Add
            addDanmaku();
            ++danmakuIter;
        }
    }

    // Move Danmaku
    for (auto &track : danmakuTracks) {
        for (auto iter = track.begin(); iter != track.end();) {

            // auto size = item.size();

            iter->x -= (width + iter->w) / danmakuAliveTime / danmakuFps;      
                  // item.setSize(size);
            // qDebug() << "Move To" << iter->x << " " << iter->h;

            if (iter->x + iter->w < -100) {
                // Out of range, drop
                iter = track.erase(iter);
            }
            else {
                ++iter;
            }
        }
    }
    // Check the danmaku still alive
    for (auto iter = danmakuTopBottomTrack.begin(); iter != danmakuTopBottomTrack.end(); ) {
        if (std::abs(position - iter->data->position) > danmakuAliveTime) {
            // Out of range, drop
            iter = danmakuTopBottomTrack.erase(iter);
        }
        else {
            ++iter;
        }
    }

    videoCanvas->update();
}
void VideoCanvasPrivate::addDanmaku() {
    auto &dan = *danmakuIter;

    QFont font = danmakuFont;
    font.setPixelSize(danmakuScale * int(dan.size));

    QFontMetricsF metrics(font);

    QSizeF size = metrics.size(Qt::TextSingleLine, dan.text);
    qreal width = size.width();
    qreal height = size.height();


    // Prepare node
    DanmakuPaintItem item;
    item.data = &*danmakuIter;
    item.text.setText(dan.text);
    item.x = 0;
    item.y = 0;
    item.w = width;
    item.h = height;
    item.font = font;

    if (dan.isRegular()) {
        // For tracks
        qreal x = videoCanvas->width();
        qreal y = 0;

        for (auto &track : danmakuTracks) {
            // 检查最后一个
            if (!track.empty()) {
                if (track.back().rect().intersects(QRectF(x, y, width, height))) {
                    // Do down
                    y += height;
                    y += danmakuSpacing;
                    continue;
                }
            }
            // Has space
            item.x = x;
            item.y = y;
            item.w = width;
            item.h = height;
            track.push_back(std::move(item));
            return;
        }
        // Drop
    }
    else if (dan.isTop()) {
        // form Top to bottom (begin to end)
        item.w = width;
        item.h = height;
        qreal x = 0;
        qreal y = 0;
        if (!danmakuTopBottomTrack.empty()) {
            for (auto iter = danmakuTopBottomTrack.begin(); iter != danmakuTopBottomTrack.end(); ++iter) {
                if (y >= iter->y + iter->h || iter->y >= y + height) {
                    // Not in range, has space
                    item.x = x;
                    item.y = y;

                    danmakuTopBottomTrack.insert(iter, std::move(item));
                    return;
                }
                y += qMax(height, iter->h);
                y += danmakuSpacing;

                if (danmakuTracksLimit != 1.0 && y + iter->h >= danmakuTracksLimit * videoCanvas->height()) {
                    // Drop on out of limits
                    return;
                }
            }
            // Check if out of the bounds
            if (y + height <= videoCanvas->height()) {
                item.x = x;
                item.y = y;

                danmakuTopBottomTrack.push_back(std::move(item));
            }
        }
        else {
            item.x = x;
            item.y = y;

            danmakuTopBottomTrack.push_front(std::move(item));
            return;
        }
        // Drop
    }
    else if (dan.isBottom() && danmakuTracksLimit == 1.0) {
        // form Bottom to top (end to begin)
        // Drop  if it has limit
        item.w = width;
        item.h = height;
        qreal x = 0;
        qreal y = videoCanvas->height() - height;
        if (!danmakuTopBottomTrack.empty()) {
            for (auto iter = danmakuTopBottomTrack.rbegin(); iter != danmakuTopBottomTrack.rend(); ++iter) {
                if (y >= iter->y + iter->h || iter->y >= y + height) {
                    // Not in range, has space
                    item.x = x;
                    item.y = y;

                    danmakuTopBottomTrack.insert(iter.base(), std::move(item));
                    return;
                }
                y -= qMax(height, iter->h);
                y -= danmakuSpacing;
            }
            // Check if out of the bounds
            if (y >= 0) {
                item.x = x;
                item.y = y;

                danmakuTopBottomTrack.push_front(std::move(item));
            }
        }
        else {
            item.x = x;
            item.y = y;

            danmakuTopBottomTrack.push_back(std::move(item));
            return;
        }
        // Drop
    }
    qDebug() << "Danmaku drop " << dan.text;
}
void VideoCanvasPrivate::clearTracks() {
    for (auto &track : danmakuTracks) {
        track.clear();
    }
    danmakuTopBottomTrack.clear();
}
QRectF VideoCanvasPrivate::viewportRect() const {
#if defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    qreal texWidth = image.width();
    qreal texHeight = image.height();
#else
    qreal texWidth = textureWidth;
    qreal texHeight = textureHeight;
#endif
    qreal winWidth = videoCanvas->width();
    qreal winHeight = videoCanvas->height();

    switch (aspectMode) {
        case VideoCanvas::Filling : return QRectF(0, 0, winWidth, winHeight);
        case VideoCanvas::KeepAspect : break;
        case VideoCanvas::_4x3 : texWidth = 4; texHeight = 4; break;
        case VideoCanvas::_16x9 : texWidth = 16; texHeight = 9; break;
    }

    qreal x, y, w, h;

    if(texWidth * winHeight > texHeight * winWidth){
        w = winWidth;
        h = texHeight * winWidth / texWidth;
    }
    else{
        w = texWidth * winHeight / texHeight;
        h = winHeight;
    }
    x = (winWidth - w) / 2;
    y = (winHeight - h) / 2;

    return QRectF(x, y, w, h);
}
void VideoCanvasPrivate::_on_playerStateChanged(NekoMediaPlayer::PlaybackState state) {
    switch (state) {
        case NekoMediaPlayer::PlayingState : {
            // Not empty, start timer
            if (!danmakuList.empty()) {
                danmakuTimer = startTimer(1000 / danmakuFps, Qt::PreciseTimer);
            }
            break;
        }
        case NekoMediaPlayer::PausedState : {
            [[fallthrough]];
        }
        case NekoMediaPlayer::StoppedState : {
            // Stop danmakuTimer if
            if (danmakuTimer != 0) {
                killTimer(danmakuTimer);
                danmakuTimer = 0;

                // clearTracks();
            }
            break;
        }
    }
}
void VideoCanvasPrivate::_on_SubtitleTextChanged(const QString &subtitle) {
    subtitleText.setText(subtitle);
    hasSubtitle = !subtitle.isEmpty();
    videoCanvas->update();
}
void VideoCanvasPrivate::_on_VideoFrameChanged(const NekoVideoFrame &frame) {
    // Update frame
    if (frame.isNull() || player->playbackState() == NekoMediaPlayer::StoppedState) {

#if defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)

#else
        videoCanvas->makeCurrent();
        // Free previously texture memory
        for (auto &t : textures) {
            if (t) {
                gl->glDeleteTextures(1, &t);
                t = 0;
            }
        }
#endif
        textureWidth = 0;
        textureHeight = 0;

        videoCanvas->update();
        return;
    }
    std::lock_guard locker(frame);

    // Q_ASSERT(frame.pixelFormat() == NekoVideoPixelFormat::RGBA32);

    int w = frame.width();
    int h = frame.height();
    int pitch = frame.bytesPerLine(0);
    int planes = frame.planeCount();
    uchar *pixels = frame.bits(0);
    auto format = frame.pixelFormat();

#if defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    if (image.isNull() || image.width() != w || image.height() != h) {
        image = QImage(w, h, QImage::Format_RGBA8888);
        image.fill(Qt::black);
    }
    uchar *dst = image.bits();
    int    dstPitch = image.bytesPerLine();

    if (dstPitch == pitch) {
        // Just memcpy
        ::memcpy(dst, pixels, pitch * h);
    }
    else {
        // Update it
        for (int y = 0; y < image.height(); y++) {
            for (int x = 0; x < image.width(); x++) {
                *((uint32_t*)   &dst[y * dstPitch + x * 4]) = *(
                    (uint32_t*) &pixels[y * pitch + x * 4]
                );
            }
        }
    }
#else
    videoCanvas->makeCurrent();
    switch (format) {
        case NekoVideoPixelFormat::RGBA32 : currentShader = Shader_RGBA; break;
        case NekoVideoPixelFormat::NV12: currentShader = Shader_NV12; break;
        case NekoVideoPixelFormat::YUV420P : currentShader = Shader_YUV420P; break;
        default: abort();
    }
    if (textures[0] == 0 || textureWidth != w || textureHeight != h) {
        textureWidth = w;
        textureHeight = h;
        // Free previously texture memory
        for (auto &t : textures) {
            if (t) {
                gl->glDeleteTextures(1, &t);
                t = 0;
            }
        }
        gl->glGenTextures(planes, textures);
        VGL_CHECK_ERROR();

        for (int n = 0; n < planes; n++) {
            gl->glBindTexture(GL_TEXTURE_2D, textures[n]);
            VGL_CHECK_ERROR();

            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        updateGLBuffer();
    }
    
    // Prepare the texture
    if (currentShader == Shader_RGBA) {
        gl->glBindTexture(GL_TEXTURE_2D, textures[0]);
        VGL_CHECK_ERROR();

        // Settings updates
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);

        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        VGL_CHECK_ERROR();

        // Restore
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        gl->glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        gl->glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    }
    else if (currentShader == Shader_YUV420P) {
        Q_ASSERT(planes == 3); // Has 3 planes

        auto yData = frame.bits(0);
        auto uData = frame.bits(1);
        auto vData = frame.bits(2);
        auto yPitch = frame.bytesPerLine(0);
        auto uPitch = frame.bytesPerLine(1);
        auto vPitch = frame.bytesPerLine(1);

        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, yPitch);
        gl->glBindTexture(GL_TEXTURE_2D, textures[0]);
        VGL_CHECK_ERROR();
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, yData);
        VGL_CHECK_ERROR();

        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, uPitch);
        gl->glBindTexture(GL_TEXTURE_2D, textures[1]);
        VGL_CHECK_ERROR();
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, uData);
        VGL_CHECK_ERROR();

        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, vPitch);
        gl->glBindTexture(GL_TEXTURE_2D, textures[2]);
        VGL_CHECK_ERROR();
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w / 2, h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, vData);
        VGL_CHECK_ERROR();

        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    else if (currentShader == Shader_NV12) {
        Q_ASSERT(planes == 2); // Has 3 planes

        auto yData = frame.bits(0);
        auto uvData = frame.bits(1);
        auto yPitch = frame.bytesPerLine(0);
        auto uvPitch = frame.bytesPerLine(1);

        // Update Y
        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, yPitch);
        gl->glBindTexture(GL_TEXTURE_2D, textures[0]);
        VGL_CHECK_ERROR();
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, yData);
        VGL_CHECK_ERROR();

        // Update UV
        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, uvPitch / 2);
        gl->glBindTexture(GL_TEXTURE_2D, textures[1]);
        VGL_CHECK_ERROR();
        gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, w / 2, h / 2, 0, GL_RG, GL_UNSIGNED_BYTE, uvData);
        VGL_CHECK_ERROR();

        gl->glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
#endif    


    videoCanvas->update();
}



// OpenGL parts

#if !defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)

static auto vertexShaderCode = R"(
#version 330 core
layout (location = 0) in vec2 inputPos;
layout (location = 1) in vec2 inputTexturePos;

out  vec2 texturePos;

void main() {
    gl_Position = vec4(inputPos.x, inputPos.y, 0.0, 1.0);
    texturePos = inputTexturePos; //< Pass to fragment shader
}

)";

static auto fragmentShaderCode = R"(
#version 330 core
out vec4 fragColor;
in  vec2 texturePos;

uniform sampler2D videoTexture;

void main(){
    fragColor = texture(videoTexture, texturePos); //< Sampling pixels from texture
}

)";

static auto yuv420PShaderCode = R"(
#version 330 core
out vec4 fragColor;
in  vec2 texturePos;

uniform sampler2D yTexture;
uniform sampler2D uTexture;
uniform sampler2D vTexture;

void main(){
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture(yTexture, texturePos).r - 0.0625;
    yuv.y = texture(uTexture, texturePos).r - 0.5;
    yuv.z = texture(vTexture, texturePos).r - 0.5;

    rgb = mat3( 1,       1,         1,
        0,       -0.39465,  2.03211,
        1.13983, -0.58060,  0) * yuv;
    fragColor = vec4(rgb, 1);
}

)";

static auto nv12ShaderCode = R"(
#version 330 core
out vec4 fragColor;
in  vec2 texturePos;

uniform sampler2D yTexture;
uniform sampler2D uvTexture;

void main(){
    vec3 yuv;
    vec3 rgb;

    yuv.x = texture(yTexture, texturePos).r - 0.0625;
    yuv.y = texture(uvTexture, texturePos).r - 0.5;
    yuv.z = texture(uvTexture, texturePos).g - 0.5;


    rgb = mat3( 1,       1,         1,
        0,       -0.39465,  2.03211,
        1.13983, -0.58060,  0) * yuv;
    fragColor = vec4(rgb, 1);
}

)";


void VideoCanvasPrivate::cleanupGL() {
    qDebug() << "VideoCanvasPrivate::cleanGL";
    
    if (vertexArrayObject) {
        gl->glDeleteVertexArrays(1, &vertexArrayObject);
        vertexArrayObject = 0;
    }
    if (vertexBufferObject) {
        gl->glDeleteBuffers(1, &vertexBufferObject);
        vertexBufferObject = 0;
    }
    for (auto &t : textures) {
        if (t) {
            gl->glDeleteTextures(1, &t);
            t = 0;
        }
    }

    for (auto &program : programObjects) {
        if (program) {
            gl->glDeleteProgram(program);
            program = 0;
        }
    }
    gl.reset();
}
void VideoCanvasPrivate::initializeGL() {
    qDebug() << "VideoCanvasPrivate::initializeGL";

    // Make vertex array
    gl->glGenVertexArrays(1, &vertexArrayObject);
    VGL_CHECK_ERROR();
    gl->glBindVertexArray(vertexArrayObject);
    VGL_CHECK_ERROR();

    // prepare vertex buffer objects
    gl->glGenBuffers(1, &vertexBufferObject);
    VGL_CHECK_ERROR();

    float vertices[] = {
        //< vertex       //< texture position
        -1.0f, 1.0f,       0.0f, 1.0f,
        1.0f,  1.0f,       1.0f, 1.0f,
        1.0f,  -1.0f,      1.0f, 0.0f,
        -1.0f, -1.0f,      0.0f, 0.0f, 
        -1.0f, 1.0f,       0.0f, 1.0f,
    };
    gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    VGL_CHECK_ERROR();

    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    VGL_CHECK_ERROR();

    // configure this buffer props
    gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    VGL_CHECK_ERROR();
    gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    VGL_CHECK_ERROR();
    gl->glEnableVertexAttribArray(0);
    VGL_CHECK_ERROR();
    gl->glEnableVertexAttribArray(1);
    VGL_CHECK_ERROR();

    // prepare shader objects
    prepareProgram(Shader_RGBA, vertexShaderCode, fragmentShaderCode);
    prepareProgram(Shader_NV12, vertexShaderCode, nv12ShaderCode);
    prepareProgram(Shader_YUV420P, vertexShaderCode, yuv420PShaderCode);
}
void VideoCanvasPrivate::prepareProgram(int type, const char *vtCode, const char *frCode) {
    GLuint programObject = gl->glCreateProgram();
    VGL_CHECK_ERROR();

    auto vertexShader = gl->glCreateShader(GL_VERTEX_SHADER);
    auto fragmentShader = gl->glCreateShader(GL_FRAGMENT_SHADER);
    int  success;
    char infoLog[512];

    gl->glShaderSource(vertexShader, 1, &vtCode, nullptr);
    gl->glCompileShader(vertexShader);
    gl->glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        gl->glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        qDebug() << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog;
    }

    gl->glShaderSource(fragmentShader, 1, &frCode, nullptr);
    gl->glCompileShader(fragmentShader);
    gl->glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        gl->glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        qDebug() << "ERROR::SHADER::FRAGM::COMPILATION_FAILED\n" << infoLog;
    }

    // Link it
    gl->glAttachShader(programObject, vertexShader);
    gl->glAttachShader(programObject, fragmentShader);
    gl->glLinkProgram(programObject);
    gl->glGetProgramiv(programObject, GL_LINK_STATUS, &success);
    if(!success) {
        gl->glGetProgramInfoLog(programObject, 512, NULL, infoLog);
        qDebug() << "ERROR::SHADER::LINK_FAILED\n" << infoLog;
    }

    // Release
    gl->glDeleteShader(vertexShader);
    gl->glDeleteShader(fragmentShader);


    // Bind uniform locations
    gl->glUseProgram(programObject);
    if (type == Shader_RGBA) {
        gl->glUniform1i(gl->glGetUniformLocation(programObject, "videoTexture"), 0);
        VGL_CHECK_ERROR();
    }
    if (type == Shader_YUV420P) {
        gl->glUniform1i(gl->glGetUniformLocation(programObject, "yTexture"), 0);
        VGL_CHECK_ERROR();
        gl->glUniform1i(gl->glGetUniformLocation(programObject, "uTexture"), 1);
        VGL_CHECK_ERROR();
        gl->glUniform1i(gl->glGetUniformLocation(programObject, "vTexture"), 2);
        VGL_CHECK_ERROR();
    }
    if (type == Shader_NV12) {
        gl->glUniform1i(gl->glGetUniformLocation(programObject, "yTexture"), 0);
        VGL_CHECK_ERROR();
        gl->glUniform1i(gl->glGetUniformLocation(programObject, "uvTexture"), 1);
        VGL_CHECK_ERROR();
    }

    programObjects[type] = programObject;
}
void VideoCanvasPrivate::paintGL() {
    gl->glClearColor(0.0, 0.0f, 0.0f, 1.0f);
    VGL_CHECK_ERROR();
    gl->glClear(GL_COLOR_BUFFER_BIT);
    VGL_CHECK_ERROR();


    // Begin drawing
    if (textures[0] == 0) {
        return;
    }

    // Active each texture unit and bind texture
    int n = 0;
    for (auto t : textures) {
        if (t == 0) {
            break;
        }
        gl->glActiveTexture(GL_TEXTURE0 + n);
        gl->glBindTexture(GL_TEXTURE_2D, t);

        n += 1;
    }
    gl->glBindVertexArray(vertexArrayObject);
    VGL_CHECK_ERROR();
    gl->glUseProgram(programObjects[currentShader]);
    VGL_CHECK_ERROR();
    
    
    gl->glDrawArrays(GL_TRIANGLES, 0, 3);
    gl->glDrawArrays(GL_TRIANGLES, 2, 3);
    VGL_CHECK_ERROR();
}
void VideoCanvasPrivate::resizeGL(int w, int h) {
    qDebug() << "GL resized to w:" << w << " h:" << h;
    if (textures[0]) {
        updateGLBuffer();
    }
}
void VideoCanvasPrivate::updateGLBuffer() {
    float width = videoCanvas->width();
    float height = videoCanvas->height();

    auto mapPoint = [=](const QPointF &p) {
        float x = p.x();
        float y = p.y();
        x = (x - (width / 2)) / (width / 2);
        y = (y - (height / 2)) / (height / 2);

        return QPointF(x, y);
    };


    QRectF outRect = viewportRect();
    auto topLeft = mapPoint(outRect.topLeft());
    auto topRight = mapPoint(outRect.topRight());
    auto bottomLeft = mapPoint(outRect.bottomLeft());
    auto bottomRight = mapPoint(outRect.bottomRight());

    float vertices[] = {                 //< Texture position
        topLeft.x(), topLeft.y(),         0.0f, 1.0f,
        topRight.x(), topRight.y(),       1.0f, 1.0f,
        bottomRight.x(), bottomRight.y(), 1.0f, 0.0f,
        bottomLeft.x(), bottomLeft.y(),   0.0f, 0.0f,  
        topLeft.x(), topLeft.y(),         0.0f, 1.0f,
    };
    gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    VGL_CHECK_ERROR();

    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    VGL_CHECK_ERROR();
}

#endif