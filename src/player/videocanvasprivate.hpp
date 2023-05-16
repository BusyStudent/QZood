#pragma once

#include "videocanvas.hpp"
#include "../nekoav/nekoav.hpp"
#include "../danmaku.hpp"
#include <QStaticText>

class DanmakuPaintItem final {
    public:
        qreal              x = 0;
        qreal              y = 0;
        qreal              w = 0;
        qreal              h = 0;
        const DanmakuItem *data; //Info
        QStaticText        text;
        QFont              font;

        QRectF rect() {
            return QRectF(x, y, w, h);
        }
};

using DanmakuTracks = std::list<std::list<DanmakuPaintItem>>;

class VideoCanvasPrivate final : public QObject {
    Q_OBJECT
    public:
        VideoCanvasPrivate(VideoCanvas *parent);

        VideoCanvas *videoCanvas = nullptr;
        NekoMediaPlayer *player = nullptr;
        NekoVideoSink videoSink;

        // OpenGL datas
        // GLuint                texture = 0;
        // GLuint                textureWidth = 0;
        // GLuint                textureHeight = 0;
        // QOpenGLShaderProgram *program = nullptr;
        // QOpenGLBuffer        *buffer = nullptr;
        // QOpenGLVertexArrayObject *vertexArray = nullptr;

        QImage              image;

        // Danmakus
        QFont               danmakuFont =  QFont("黑体");
        int                 danmakuTimer = 0;
        qreal               danmakuFps   = 60;
        qreal               danmakuScale = 0.8; //< Scale factor for danmaku.  1.0 = 100% scale.  0.0 = normal scale.
        qreal               danmakuAliveTime = 10.0; //< Alive of a danmaku
        qreal               danmakuSpacing = 2.0; //< Spacing 
        bool                danmakuPlaying = false; //< Is danmaku playing?
        DanmakuList         danmakuList; //< The list of danmaku to display.
        DanmakuTracks       danmakuTracks; //< The list of QGraphicsTextItem to display.  Each QTextItem is a danmaku.
        DanmakuList::const_iterator danmakuIter; //< The iterator of current position

        void paint(QPainter &);
        void paintDanmaku(QPainter &);
        void resizeTracks();
    protected:
        void timerEvent(QTimerEvent *) override;
    private:
        void addDanmaku();
        void _on_VideoFrameChanged(const NekoVideoFrame &frame);
        void _on_playerStateChanged(NekoMediaPlayer::PlaybackState status);
    friend class VideoCanvas;
};