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

        void setDanmakuOpacity(double op);
        void setDanmakuFont(const QFont &font);
        void setDanmakuList(const DanmakuList &d);
        void setDanmakuPosition(qreal position);

        qreal danmakuOpacity() const;
    protected:
        void paintGL() override;
        void resizeGL(int w, int h) override;
        // void paintEvent(QPaintEvent *) override;
        void resizeEvent(QResizeEvent *) override;
    private:
        QScopedPointer<VideoCanvasPrivate> d;
};