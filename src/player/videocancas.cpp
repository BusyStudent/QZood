#include "videocanvas.hpp"
#include "videocanvasprivate.hpp"

#include "../log.hpp"
#include <QTimerEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextCursor>
#include <QPainter>
#include <mutex>


VideoCanvas::VideoCanvas(QWidget *parent) : QOpenGLWidget(parent), d(new VideoCanvasPrivate(this)) {

}
VideoCanvas::~VideoCanvas() {

}
void VideoCanvas::attachPlayer(NekoMediaPlayer *player) {
    d->player = player;

    player->setVideoSink(&d->videoSink);

    connect(player, &NekoMediaPlayer::playbackStateChanged, d.get(), &VideoCanvasPrivate::_on_playerStateChanged);
}
void VideoCanvas::setDanmakuList(const DanmakuList &list) {
    Q_ASSERT(d->player);
    d->danmakuList = list;
    d->danmakuIter = d->danmakuList.cbegin();

    if (d->danmakuTimer == 0 && d->player->playbackState() == NekoMediaPlayer::PlayingState) {
        // Start timer
        d->danmakuTimer = d->startTimer(1000 / d->danmakuFps, Qt::PreciseTimer);
    }
}
void VideoCanvas::setDanmakuPosition(qreal position) {
    for (auto &track : d->danmakuTracks) {
        track.clear();
    }
    d->danmakuTracks.clear();

    d->danmakuIter = d->danmakuList.begin();
    while (d->danmakuIter->position < position && d->danmakuIter != d->danmakuList.cend()) {
        ++(d->danmakuIter);
    }
    if (d->danmakuIter != d->danmakuList.cend()) {
        qDebug() << "Seek to pos: " << d->danmakuIter->position << " text: " << d->danmakuIter->text;
    }

}
void VideoCanvas::paintGL() {
    QPainter painter(this);

    painter.fillRect(0, 0, width(), height(), Qt::black);
    d->paint(painter);
}
void VideoCanvas::resizeEvent(QResizeEvent *e) {
    QOpenGLWidget::resizeEvent(e);
    d->resizeTracks();
}
void VideoCanvas::resizeGL(int w, int h) {
    auto fns = QOpenGLContext::currentContext()->functions();

    fns->glViewport(0, 0, w, h);
}

VideoCanvasPrivate::VideoCanvasPrivate(VideoCanvas *parent) : QObject(parent), videoCanvas(parent) {
    connect(&videoSink, &NekoVideoSink::videoFrameChanged, this, &VideoCanvasPrivate::_on_VideoFrameChanged, Qt::QueuedConnection);
}
void VideoCanvasPrivate::paint(QPainter &painter) {
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
    if (player) {
        painter.setFont(videoCanvas->font());
        painter.setPen(Qt::white);
        painter.drawText(0, 0, videoCanvas->width(), videoCanvas->height(), Qt::AlignTop,
            QString::asprintf("Progress %lf to %lf", 
                player->position(), player->duration()
            )
        );
    }

    // Then paint danmaku
    paintDanmaku(painter);
}
void VideoCanvasPrivate::paintDanmaku(QPainter &painter) {
    for (auto &tracks : danmakuTracks) {
        for (auto &node : tracks) {
            painter.setFont(node.font);

            painter.setPen(Qt::gray);
            painter.drawStaticText(node.x + 1, node.y + 1, node.text);

            painter.setPen(node.data->color);
            painter.drawStaticText(node.x, node.y, node.text);
        }
    }
}
void VideoCanvasPrivate::resizeTracks() {
    qreal height = videoCanvas->height();
    int num = height / DanmakuItem::Medium * danmakuScale;
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
            qDebug() << diff;
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
            qDebug() << "Move To" << iter->x << " " << iter->h;

            if (iter->x + iter->w < -100) {
                // Out of range, drop
                iter = track.erase(iter);
            }
            else {
                ++iter;
            }
        }
    }
    videoCanvas->update();
}
void VideoCanvasPrivate::addDanmaku() {
    auto &dan = *danmakuIter;

    QFont font = danmakuFont;
    font.setPointSize(danmakuScale * int(dan.size));

    QFontMetrics metrics(font);

    QSize size = metrics.size(Qt::TextSingleLine, dan.text);
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
            }
            break;
        }
    }
}
void VideoCanvasPrivate::_on_VideoFrameChanged(const NekoVideoFrame &frame) {
    // Update frame
    std::lock_guard locker(frame);

    Q_ASSERT(frame.pixelFormat() == NekoVideoPixelFormat::RGBA32);

    int w = frame.width();
    int h = frame.height();
    int pitch = frame.bytesPerLine(0);
    uchar *pixels = frame.bits(0);
    if (image.isNull() || image.width() != w || image.height() != h) {
        image = QImage(w, h, QImage::Format_RGBA8888);
        image.fill(Qt::black);
    }
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
    

    videoCanvas->update();
}