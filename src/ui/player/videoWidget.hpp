#pragma once

#include <QWidget>

#include "../../BLL/data/videoBLL.hpp"

class VideoWidgetPrivate;

enum class Order {
    IN_ORDER,
    LIST_LOOP,
    LIST_RANDOM,
    SINGLE_CYCLE,
    STOP,
};

enum class ScalingMode {
    NONE,
    _4X3,
    _16X9,
    FILLING,
};

enum class Rotation {
    COLOCKWISE,
    ANTICLOCKWISE,
    HORIZEON_FILP,
    VERTICALLY_FILP,
};

enum class StrokeType {
    NONE,
    PROJECT,
    STROKE,
};

class VideoWidget : public QWidget{
    Q_OBJECT
    public:
        VideoWidget(QWidget* parent = nullptr);
        ~VideoWidget();
        // 播放设置
        int skipStep();
        void setSkipStep(int v);
        void setPlaybackRate(qreal v);
        // 画面设置
        void setAspectRationMode(ScalingMode mode);
        void RotationScreen(Rotation direction);
        void setImageQualityEnhancement(bool v);
        // 色彩设置
        void setBrightness(int v);
        void setContrast(int v);
        void setHue(int v);
        void setSaturation(int v);
        // 弹幕设置
        void setDanmaku(const QString& danmakuSource);
        void setDanmakuShowArea(qreal OccupationRatio);
        void setDanmakuSize(qreal ratio);
        void setDanmakuSpeed(int speed);
        void setDanmakuFont(const QFont& font);
        QFont danmakuFont();
        void setDanmakuTransparency(qreal percentage);
        void setDanmakuStroke(StrokeType stroke);
        // 字幕设置
        void setSubtitle(int);
        void setSubtitleSynchronizeTime(qreal t);
        void setSubtitlePosition(qreal t);
        QFont subtitleFont();
        void setSubtitleFont(const QFont& font);
        void setSubtitleColor(const QColor& color);
        void setSubtitleTransparency(qreal percentag);
        void setSubtitleStroke(bool v);
        void setSubtitleStrokeColor(const QColor& color);
        void setSubtitleStrokeTransparency(qreal percentage);
        // 当前指针
        VideoBLLPtr currentVideo();

    public:
        void resizeEvent(QResizeEvent* event) override;
        bool eventFilter(QObject *obj, QEvent *event) override;
        void leaveEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        bool event(QEvent *event) override;

    Q_SIGNALS:
        void playing();
        void paused();
        void stoped();
        void finished();
        void invalidVideo(const VideoBLLPtr video);
        void nextVideo();
        void previousVideo();

    public Q_SLOTS:
        void playVideo(const VideoBLLPtr video);
        void videoLog(const QString& msg);
        void stop();
        // void setDanmaku(int id);
        // void setSubtitle();

    private:
        QScopedPointer<VideoWidgetPrivate> d;
};