#pragma once

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsVideoItem>
#include <QGraphicsProxyWidget>
#include <QVideoWidget>
#include <QMediaPlayer>

#include "../danmaku.hpp"
#include <vector>
#include <list>


class GraphicsDanmakuItem final : public QGraphicsTextItem {
    public:
        using QGraphicsTextItem::QGraphicsTextItem;
};

using DanmakuTracks = std::vector<std::list<GraphicsDanmakuItem *>>;

/**
 * @brief Canvas for Display danmaku & video
 * 
 */
class VideoCanvas final : public QGraphicsView {
    Q_OBJECT
    public:
        VideoCanvas(QWidget *parent = nullptr);
        ~VideoCanvas();

        void attachPlayer(QMediaPlayer *player);

        void setDanmakuOpacity(double op);
        void setDanmakuFont(const QFont &font);
        void setDanmakuList(const DanmakuList &d);

        qreal danmakuOpacity() const;
    protected:
        void timerEvent(QTimerEvent *) override;
        void resizeEvent(QResizeEvent *) override;
    private:
        void _on_videoItemSizeChanged(const QSizeF &size);
        void _on_playerStateChanged(QMediaPlayer::PlaybackState status);
    private:
        void runDanmaku(); //< Do danmaku suffer
        void addDanmaku(); //< Insert danmaku to
        void seekDanmaku(qreal pos); //< Seek danmaku position
        void putVideoItem(); //< Put the video item to current place
        void makeDanmakuTrack(); //< Calc the track of it

        QGraphicsScene     *scene = nullptr;
        QGraphicsVideoItem *item = nullptr;  //< Display item for the canvas.
        QMediaPlayer       *player = nullptr;
        QGraphicsItemGroup *danmakuGroup = nullptr; //< Place of group
        QGraphicsItemGroup *normalDanmakuGroup = nullptr; //< Place of normal danmaku
        // QVideoWidget         *widget = nullptr;
        // QGraphicsProxyWidget *widgetProxy = nullptr;
        QGraphicsTextItem    *progressText = nullptr;
        QSizeF                nativeItemSize {0.0, 0.0};
        qint64                playerPosition = 0;


        // Danmakus
        QFont               danmakuFont =  QFont("黑体");
        int                 danmakuTimer = 0;
        qreal               danmakuFps   = 60;
        qreal               danmakuScale = 0.8; //< Scale factor for danmaku.  1.0 = 100% scale.  0.0 = normal scale.
        qreal               danmakuAliveTime = 10.0; //< Alive of a danmaku
        bool                danmakuPlaying = false; //< Is danmaku playing?
        DanmakuList         danmakuList; //< The list of danmaku to display.
        // DanmakuTracks       danmakuTracks; //< The list of QGraphicsTextItem to display.  Each QTextItem is a danmaku.
        DanmakuList::const_iterator danmakuIter; //< The iterator of current position
};