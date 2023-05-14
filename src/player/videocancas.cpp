#include "videocanvas.hpp"
#include "../log.hpp"
#include <QTimerEvent>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextCursor>

VideoCanvas::VideoCanvas(QWidget *parent) : QGraphicsView(parent) {
    // Hide the border and set background to black
    setStyleSheet("border:0px; background-color: rgba(0, 0, 0, 255);");

    // Prepare scene
    scene = new QGraphicsScene(this);
    setScene(scene);

    // Add Item
    item = new NekoGraphicsVideoItem();
    scene->addItem(item);

    // widget = new QVideoWidget();
    // widget->setStyleSheet("background-color: rgba(0, 0, 0, 255)");
    // widgetProxy = scene->addWidget(widget);

    danmakuGroup = new QGraphicsItemGroup();
    scene->addItem(danmakuGroup);

    normalDanmakuGroup = new QGraphicsItemGroup();
    danmakuGroup->addToGroup(normalDanmakuGroup);
    
#if !defined(NDEBUG)
    progressText = new QGraphicsTextItem;
    scene->addItem(progressText);

    progressText->setPlainText("Progress 0 to 0");
    progressText->setPos(0, 0);
    progressText->setDefaultTextColor(Qt::white);
    progressText->setFont(font());
#endif

    danmakuFont.setBold(true);

    connect(item, &NekoGraphicsVideoItem::nativeSizeChanged, this, &VideoCanvas::_on_videoItemSizeChanged);
}
VideoCanvas::~VideoCanvas() {
    
}
void VideoCanvas::attachPlayer(NekoMediaPlayer *pl) {
    player = pl;
    player->setVideoOutput(item);
    // player->setVideoOutput(widget);

    // Connect the media status , for displaying danmaku
    connect(player, &NekoMediaPlayer::playbackStateChanged, this, &VideoCanvas::_on_playerStateChanged);

    connect(player, &NekoMediaPlayer::positionChanged, [this](qreal position) {
        // Do check here
        if (!danmakuList.empty()) {
            if (position < playerPosition) {
                // Back
                seekDanmaku(position);
            }
            if (position - playerPosition > 2000) {
                // Bigger than 2 ms, is seeking
                seekDanmaku(position);
            }
        }

        playerPosition = position;

#if !defined(NDEBUG)
        auto text = QString::asprintf("Progress %lf to %lf", position, player->duration());
        if (danmakuTimer != 0 && danmakuIter != danmakuList.cend()) {
            // Has danmaku
            text += QString("\nTracks %1 Danmaku %2 %3").arg(
                // QString::asprintf("%d", int(danmakuTracks.size())),
                "0",
                danmakuIter->text, 
                QString::asprintf("%lf", danmakuIter->position)
            );
        }
        progressText->setPlainText(text);
#endif

    });    // for updating progress
}
void VideoCanvas::_on_videoItemSizeChanged(const QSizeF &size) {
    nativeItemSize = size;
    putVideoItem();
}
void VideoCanvas::_on_playerStateChanged(NekoMediaPlayer::PlaybackState state) {
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
void VideoCanvas::putVideoItem() {
    if (!player) {
        return;
    }
    if (nativeItemSize.width() == 0 || nativeItemSize.height() == 0) {
        item->hide();
        return;
    }
    scene->setSceneRect(0, 0, width(), height());

    qreal tex_w = nativeItemSize.width();
    qreal tex_h = nativeItemSize.height();
    qreal win_w = width();
    qreal win_h = height();

    qreal x, y, w, h;

    if(tex_w * win_h > tex_h * win_w){
        w = win_w;
        h = tex_h * win_w / tex_w;
    }
    else{
        w = tex_w * win_h / tex_h;
        h = win_h;
    }
    x = (win_w - w) / 2;
    y = (win_h - h) / 2;

    // Size the item Size
    item->setPos(x, y);
    item->setSize(QSizeF(w, h));
    item->show();
}
void VideoCanvas::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);

    scene->setSceneRect(0, 0, width(), height());
    
    putVideoItem();
    // widgetProxy->setPos(0, 0);
    // widgetProxy->resize(width(), height());

    makeDanmakuTrack();
}
void VideoCanvas::timerEvent(QTimerEvent *event) {
    if (event->timerId() == danmakuTimer) {
        runDanmaku();
    }
    else {
        QGraphicsView::timerEvent(event);
    }
}


// Danmakus
void VideoCanvas::runDanmaku() {
    qreal position = player->position();
    qreal width = scene->width();

    if (danmakuIter != danmakuList.cend() && danmakuIter->position < position) {
        // Add Danmaku
        addDanmaku();
        ++danmakuIter;
    }

    // Move Danmaku
    // for (auto &track : danmakuTracks) {
    //     for (auto iter = track.begin(); iter != track.end();) {
    //         auto item = *iter;
    //         auto pos = item->pos();
    //         auto size = item->boundingRect().size();

    //         pos.setX(pos.x() - (width + size.width()) / danmakuAliveTime / danmakuFps );

    //         item->setPos(pos);

    //         qDebug() << "Move " << pos;

    //         if (pos.x() + size.width() < -100) {
    //             // Out of range, drop
    //             qDebug() << "Drop " << pos;
    //             iter = track.erase(iter);

    //             danmakuGroup->removeFromGroup(item);
    //             delete item;
    //         }
    //         else {
    //             ++iter;
    //         }
    //     }
    // }
    for (const auto item : normalDanmakuGroup->childItems()) {
        auto pos = item->pos();
        auto size = item->boundingRect().size();

        
        pos.setX(pos.x() - (width + size.width()) / danmakuAliveTime / danmakuFps );

        item->setPos(pos);


        if (pos.x() + size.width() < -100) {
            // Out of range, drop
            // qDebug() << "Drop " << pos;

            normalDanmakuGroup->removeFromGroup(item);
            delete item;
        }
    }

}
void VideoCanvas::addDanmaku() {
    const auto &danmaku = *danmakuIter;

    danmakuFont.setPointSizeF(danmakuIter->size * danmakuScale);

    // Prepare item
    auto item = new GraphicsDanmakuItem();
    item->setFont(danmakuFont);
    item->setPlainText(danmaku.text);
    item->setDefaultTextColor(danmaku.color);

    // auto doc = new QTextDocument(item);
    // QTextCharFormat format;
    // format.setTextOutline(QPen(Qt::black, 1, Qt::SolidLine));
    // format.setForeground(danmaku.color);
    // QTextCursor textCursor(doc);
    // textCursor.insertText(danmaku.text, format);
    // item->setDocument(doc);

    auto itemSize = item->boundingRect().size();

    if (danmaku.isRegular()) {
        // From Top Right
        qreal x = width();
        qreal y = 0;

        // for (auto &track : danmakuTracks) {
        //     if (!track.empty()) {
        //         // bool i = track.back()->boundingRect().intersects(QRectF(x, y, itemSize.width(), itemSize.height()));
        //         if (danmakuGroup->contains(QPointF(x, y))) {
        //             // To bottom
        //             y += track.back()->boundingRect().height();
        //             continue;
        //         }
        //     }
        //     // Got position, add it
        //     qDebug() << "Add " << x << " " << y;
        //     item->setPos(x, y);
        //     track.push_back(item);
        //     danmakuGroup->addToGroup(item);
        //     return;
        // }
        int tracks = scene->height() / itemSize.height();
        for (int i = 0; i < tracks; i++) {
            if (normalDanmakuGroup->contains(QPointF(x, y))) {
                y += itemSize.height();
                continue;
            }
            item->setPos(x, y);
            normalDanmakuGroup->addToGroup(item);
            return;
        }
        // Drop
        qDebug() << "Drop" << x << " " << y;
    }
    delete item;
    return;
}
void VideoCanvas::makeDanmakuTrack() {
    // qreal height = scene->height();
    // int num = height / DanmakuItem::Medium * danmakuScale;

    // if (num < danmakuTracks.size()) {
    //     for (int current = num; current < danmakuTracks.size(); current ++) {
    //         for (auto item : danmakuTracks[current]) {
    //             danmakuGroup->removeFromGroup(item);

    //             delete item;
    //         }
    //     }
    // }

    // danmakuTracks.resize(num);
}
void VideoCanvas::seekDanmaku(qreal pos) {
    qDeleteAll(normalDanmakuGroup->childItems());

    danmakuIter == danmakuList.cbegin();
    if (danmakuIter != danmakuList.cend() && danmakuIter->position < pos) {
        // Add Danmaku
        ++danmakuIter;
    }

    if (danmakuIter != danmakuList.cend()) {
        ZOOD_CLOG("DanmakuSeek To %lf", danmakuIter->position);
    }
}
void VideoCanvas::setDanmakuOpacity(qreal op) {
    danmakuGroup->setOpacity(op);
}
void VideoCanvas::setDanmakuList(const DanmakuList &list) {
    Q_ASSERT(player);
    danmakuList = list;
    danmakuIter = list.cbegin();

    if (danmakuTimer == 0 && player->playbackState() == NekoMediaPlayer::PlayingState) {
        // Start timer
        danmakuTimer = startTimer(1000 / danmakuFps, Qt::PreciseTimer);
    }
}
qreal VideoCanvas::danmakuOpacity() const {
    return danmakuGroup->opacity();
}