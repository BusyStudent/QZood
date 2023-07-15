#pragma once

#include "videocanvas.hpp"
#include "../nekoav/nekoav.hpp"
#include "../danmaku.hpp"

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLContext>
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
using DanmakuTrack  = std::list<DanmakuPaintItem>;

class VideoCanvasPrivate final : public QObject {
    Q_OBJECT
    public:
        using GLFunctions = QScopedPointer<QOpenGLFunctions_3_3_Core>;
        VideoCanvasPrivate(VideoCanvas *parent);

        enum ShaderType {
            Shader_RGBA = 0,
            Shader_YUV420P = 1,
            Shader_NV12 = 2,
            Shader_NbFormats,
        };

        VideoCanvas *videoCanvas = nullptr;
        NekoMediaPlayer *player = nullptr;
        NekoVideoSink videoSink;

        // OpenGL datas
        GLuint vertexArrayObject = 0;
        GLuint vertexBufferObject = 0;
        GLuint textures[4] {}; //< All planes texture
        GLuint textureWidth = 0;
        GLuint textureHeight = 0;
        GLuint programObjects[Shader_NbFormats] {};

        int    currentShader = 0; //< Index of current shader
        GLFunctions gl; //< OpenGL Functions

        QImage              image;

        QStaticText         subtitleText;
        QFont               subtitleFont = QFont("黑体", 40);
        qreal               subtitleOpacity = 1.0f;
        QColor              subtitleColor = Qt::white;
        QColor              subtitleOutlineColor = Qt::gray;
        bool                hasSubtitle = false;

        // Danmakus
        QFont               danmakuFont = QFont("黑体");
        int                 danmakuTimer = 0;
        qreal               danmakuFps   = 60;
        qreal               danmakuScale = 0.8; //< Scale factor for danmaku.  1.0 = 100% scale.  0.0 = normal scale.
        qreal               danmakuAliveTime = 8.0; //< Alive of a danmaku
        qreal               danmakuSpacing = 6.0; //< Spacing 
        qreal               danmakuOpacity = 0.8; //< Opacity
        qreal               danmakuTracksLimit = 1.0; //< Limit Ratio
        bool                danmakuPlaying = false; //< Is danmaku playing?
        bool                danmakuVisible = true; //< Is danmaku visible?
        DanmakuList         danmakuList; //< The list of danmaku to display.
        DanmakuTracks       danmakuTracks; //< The list of QGraphicsTextItem to display.  Each QTextItem is a danmaku.
        DanmakuTrack        danmakuTopBottomTrack; //< Botttom danmaku
        DanmakuList::const_iterator danmakuIter; //< The iterator of current position
        VideoCanvas::ShadowMode     danmakuShadowMode = VideoCanvas::Projection;

        VideoCanvas::AspectMode     aspectMode = VideoCanvas::KeepAspect;

        void paint(QPainter &);
        void paintDanmaku(QPainter &);
        void resizeTracks();
        void clearTracks();

        void initializeGL();
        void paintGL();
        void cleanupGL();
        void resizeGL(int w, int h);
        void updateGLBuffer();
        void prepareProgram(int type, const char *vtCode, const char *frCode);

        /**
         * @brief Get the texture puted rectangles
         * 
         * @return QRectF 
         */
        QRectF viewportRect() const;
    protected:
        void timerEvent(QTimerEvent *) override;
    private:
        void addDanmaku();
        void _on_VideoFrameChanged(const NekoVideoFrame &frame);
        void _on_SubtitleTextChanged(const QString &text);
        void _on_playerStateChanged(NekoMediaPlayer::PlaybackState status);
    friend class VideoCanvas;
};