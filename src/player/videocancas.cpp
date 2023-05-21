#include "videocanvas.hpp"
#include "videocanvasprivate.hpp"

#include "../log.hpp"
#include <QTimerEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QFontMetricsF>
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

    d->danmakuIter = d->danmakuList.begin();
    while (d->danmakuIter->position < position && d->danmakuIter != d->danmakuList.cend()) {
        ++(d->danmakuIter);
    }
    if (d->danmakuIter != d->danmakuList.cend()) {
        qDebug() << "Seek to pos: " << d->danmakuIter->position << " text: " << d->danmakuIter->text;
    }
    update();
}
void VideoCanvas::setDanmakuOpacity(qreal op) {
    d->danmakuOpacity = op;
    update();    
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
        
        if (danmakuIter != danmakuList.cend()) {
            painter.drawText(0, 0, videoCanvas->width(), videoCanvas->height(), Qt::AlignBottom,
                QString::asprintf("Tracks %d Progress %lf to %lf Danmaku %lf %s", 
                    int(danmakuTracks.size()), player->position(), player->duration(),
                    danmakuIter->position, danmakuIter->text.toUtf8().constData()
                )
            );
        }
        else {
            painter.drawText(0, 0, videoCanvas->width(), videoCanvas->height(), Qt::AlignBottom,
                QString::asprintf("Tracks %d Progress %lf to %lf", 
                    int(danmakuTracks.size()), player->position(), player->duration()
                )
            );
        }
    }

    // Then paint danmaku
    paintDanmaku(painter);
}
void VideoCanvasPrivate::paintDanmaku(QPainter &painter) {
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
    else if (dan.isBottom()) {
        // form Bottom to top (end to begin)
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
    

    videoCanvas->update();
}