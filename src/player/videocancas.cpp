#include "videocanvas.hpp"
#include "videocanvasprivate.hpp"

#include "../log.hpp"
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
    #define VGL_CHECK_ERROR() _vglCheckError(this, __FILE__, __LINE__)
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
qreal VideoCanvas::danmakuTracksLimit() const {
    return d->danmakuTracksLimit;
}
qreal VideoCanvas::danmakuOpacity() const {
    return d->danmakuOpacity;
}
QFont VideoCanvas::danmakuFont() const {
    return d->danmakuFont;
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
    d->initializeOpenGLFunctions();
    d->initializeGL();

    // Set GL cleanup function
    QObject::connect(ctxt, &QOpenGLContext::aboutToBeDestroyed, d.get(), &VideoCanvasPrivate::cleanupGL);
#endif
}

VideoCanvasPrivate::VideoCanvasPrivate(VideoCanvas *parent) : QObject(parent), videoCanvas(parent) {
    connect(&videoSink, &NekoVideoSink::videoFrameChanged, this, &VideoCanvasPrivate::_on_VideoFrameChanged, Qt::QueuedConnection);
}
void VideoCanvasPrivate::paint(QPainter &painter) {
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

#if defined(QZOOD_VIDEO_NO_CUSTOMIZE_OPENGL)
    if (!image.isNull()) {
        qreal texWidth = image.width();
        qreal texHeight = image.height();
        qreal winWidth = videoCanvas->width();
        qreal winHeight = videoCanvas->height();

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

        // Size the item Size

        painter.drawImage(QRectF(x, y, w, h), image);
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
void VideoCanvasPrivate::_on_VideoFrameChanged(const NekoVideoFrame &frame) {
    // Update frame
    if (frame.isNull() || player->playbackState() == NekoMediaPlayer::StoppedState) {
        return;
    }
    std::lock_guard locker(frame);

    // Q_ASSERT(frame.pixelFormat() == NekoVideoPixelFormat::RGBA32);

    int w = frame.width();
    int h = frame.height();
    int pitch = frame.bytesPerLine(0);
    uchar *pixels = frame.bits(0);

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
    if (texture == 0 || textureWidth != w || textureHeight != h) {
        textureWidth = w;
        textureHeight = h;
        if (texture) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
        glGenTextures(1, &texture);
        VGL_CHECK_ERROR();
        glActiveTexture(GL_TEXTURE0);
        VGL_CHECK_ERROR();
        glBindTexture(GL_TEXTURE_2D, texture);
        VGL_CHECK_ERROR();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        updateGLBuffer();
    }
    else {
        glBindTexture(GL_TEXTURE_2D, texture);
        VGL_CHECK_ERROR();
    }
    
    // Update texture
    glActiveTexture(GL_TEXTURE0);
    VGL_CHECK_ERROR();

    // Settings updates
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    VGL_CHECK_ERROR();

    // Restore
    // glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
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


void VideoCanvasPrivate::cleanupGL() {
    qDebug() << "VideoCanvasPrivate::cleanGL";
    
    if (vertexArrayObject) {
        glDeleteVertexArrays(1, &vertexArrayObject);
        vertexArrayObject = 0;
    }
    if (vertexBufferObject) {
        glDeleteBuffers(1, &vertexBufferObject);
        vertexBufferObject = 0;
    }
    if (programObject) {
        glDeleteProgram(programObject);
        programObject = 0;
    }
    if (texture) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }
}
void VideoCanvasPrivate::initializeGL() {
    qDebug() << "VideoCanvasPrivate::initializeGL";

    // Make vertex array
    glGenVertexArrays(1, &vertexArrayObject);
    VGL_CHECK_ERROR();
    glBindVertexArray(vertexArrayObject);
    VGL_CHECK_ERROR();

    // prepare vertex buffer objects
    glGenBuffers(1, &vertexBufferObject);
    VGL_CHECK_ERROR();

    float vertices[] = {
        //< vertex       //< texture position
        -1.0f, 1.0f,       0.0f, 1.0f,
        1.0f,  1.0f,       1.0f, 1.0f,
        1.0f,  -1.0f,      1.0f, 0.0f,
        -1.0f, -1.0f,      0.0f, 0.0f, 
        -1.0f, 1.0f,       0.0f, 1.0f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    VGL_CHECK_ERROR();

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    VGL_CHECK_ERROR();

    // configure this buffer props
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
    VGL_CHECK_ERROR();
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    VGL_CHECK_ERROR();
    glEnableVertexAttribArray(0);
    VGL_CHECK_ERROR();
    glEnableVertexAttribArray(1);
    VGL_CHECK_ERROR();

    // prepare shader objects
    programObject = glCreateProgram();
    VGL_CHECK_ERROR();

    auto vertexShader = glCreateShader(GL_VERTEX_SHADER);
    auto fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    int  success;
    char infoLog[512];

    glShaderSource(vertexShader, 1, &vertexShaderCode, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        qDebug() << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog;
    }

    glShaderSource(fragmentShader, 1, &fragmentShaderCode, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        qDebug() << "ERROR::SHADER::FRAGM::COMPILATION_FAILED\n" << infoLog;
    }

    // Link it
    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    glLinkProgram(programObject);
    glGetProgramiv(programObject, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(programObject, 512, NULL, infoLog);
        qDebug() << "ERROR::SHADER::LINK_FAILED\n" << infoLog;
    }

    // Release
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);


    // Bind uniform locations
    glUseProgram(programObject);
    glUniform1i(glGetUniformLocation(programObject, "videoTexture"), 0);
    VGL_CHECK_ERROR();
}
void VideoCanvasPrivate::paintGL() {
    glClearColor(0.0, 0.0f, 0.0f, 1.0f);
    VGL_CHECK_ERROR();
    glClear(GL_COLOR_BUFFER_BIT);
    VGL_CHECK_ERROR();


    // Begin drawing
    if (texture == 0) {
        return;
    }
    glActiveTexture(GL_TEXTURE0);
    VGL_CHECK_ERROR();
    glBindTexture(GL_TEXTURE_2D, texture);
    VGL_CHECK_ERROR();
    glBindVertexArray(vertexArrayObject);
    VGL_CHECK_ERROR();
    glUseProgram(programObject);
    VGL_CHECK_ERROR();
    
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDrawArrays(GL_TRIANGLES, 2, 3);
    VGL_CHECK_ERROR();
}
void VideoCanvasPrivate::resizeGL(int w, int h) {
    qDebug() << "GL resized to w:" << w << " h:" << h;
    if (texture) {
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

    qreal winWidth = videoCanvas->width();
    qreal winHeight = videoCanvas->height();

    qreal x, y, w, h;

    if(textureWidth * height > textureHeight * width){
        w = width;
        h = textureHeight * width / textureWidth;
    }
    else{
        w = textureWidth * height / textureHeight;
        h = height;
    }
    x = (width - w) / 2;
    y = (height - h) / 2;

    QRectF outRect(x, y, w, h);
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
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    VGL_CHECK_ERROR();

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    VGL_CHECK_ERROR();
}

#endif