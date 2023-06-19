#pragma once


#include "../danmaku.hpp"
#include "../nekoav/nekoav.hpp"
#include <QOpenGLWidget>

class VideoCanvasPrivate;
class VideoCanvas final : public QOpenGLWidget {
    Q_OBJECT
    public:
        VideoCanvas(QWidget *parent = nullptr);
        ~VideoCanvas();

        void attachPlayer(NekoMediaPlayer *player);

        void setDanmakuOpacity(qreal op);
        void setDanmakuFont(const QFont &font);
        void setDanmakuList(const DanmakuList &d);
        void setDanmakuPosition(qreal position);
        void setDanmakuVisible(bool visible);
        void setDanmakuTracksLimit(qreal limit);

        qreal danmakuOpacity() const;
        QFont danmakuFont() const;
        qreal danmakuTracksLimit() const;
    protected:
        void paintGL() override;
        void resizeGL(int w, int h) override;
        void initializeGL() override;
        // void paintEvent(QPaintEvent *) override;
        void resizeEvent(QResizeEvent *) override;
    private:
        QScopedPointer<VideoCanvasPrivate> d;
};