#pragma once


#include "../danmaku.hpp"
#include "../nekoav/nekoav.hpp"
#include <QOpenGLWidget>

class VideoCanvasPrivate;
class VideoCanvas final : public QOpenGLWidget {
    Q_OBJECT
    public:
        enum AspectMode {
            Filling,
            _4x3,
            _16x9,
            KeepAspect,
        };
        enum ShadowMode {
            Projection,
            Outline
        };
        Q_ENUM(AspectMode)

        VideoCanvas(QWidget *parent = nullptr);
        ~VideoCanvas();

        void attachPlayer(NekoMediaPlayer *player);

        void setDanmakuOpacity(qreal op);
        void setDanmakuFont(const QFont &font);
        void setDanmakuList(const DanmakuList &d);
        void setDanmakuPosition(qreal position);
        void setDanmakuVisible(bool visible);
        void setDanmakuTracksLimit(qreal limit);
        void setDanmakuAliveTime(qreal time);
        void setDanmakuShadowMode(ShadowMode mode);

        void setSubtitleFont(const QFont &font);
        void setSubtitleOpacity(qreal op);
        void setSubtitleColor(const QColor &color);
        void setSubtitleOutlineColor(const QColor &color);

        void setAspectMode(AspectMode mode);

        qreal danmakuOpacity() const;
        QFont danmakuFont() const;
        qreal danmakuTracksLimit() const;

        QFont subtitleFont() const;
        qreal subtitleOpacity() const;
        QColor subtitleColor() const;
        QColor subtitleOutlineColor() const;

        AspectMode aspectMode() const;
        ShadowMode danmakuShadowMode() const;
    protected:
        void paintGL() override;
        void resizeGL(int w, int h) override;
        void initializeGL() override;
        // void paintEvent(QPaintEvent *) override;
        void resizeEvent(QResizeEvent *) override;
    private:
        QScopedPointer<VideoCanvasPrivate> d;
};