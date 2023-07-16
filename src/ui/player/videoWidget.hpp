#pragma once

#include <QWidget>

#include "../../BLL/data/videoBLL.hpp"
#include "../../player/videocanvas.hpp"

class VideoWidgetPrivate;
class VideoWidget;
class VideoWidgetStatus;
class EmptyStatus;
class LoadingStatus;
class ReadyStatus;
class PlayingStatus;
class PauseStatus;

enum class Order {
    IN_ORDER,
    LIST_LOOP,
    LIST_RANDOM,
    SINGLE_CYCLE,
    STOP,
};

class VideoWidget : public QWidget{
    Q_OBJECT
    public:
        VideoWidget(QWidget* parent = nullptr);
        ~VideoWidget();
        VideoCanvas* videoCanvas();
        NekoMediaPlayer* player();
        VideoWidgetStatus* status();
        // 播放设置
        int skipStep();
        void setSkipStep(int v);
        void setPlaybackRate(qreal v);
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

    protected:
        void changeStatus(VideoWidgetStatus* status);

    private:
        QScopedPointer<VideoWidgetPrivate> d;
        QScopedPointer<VideoWidgetStatus> mStatus;

    friend class VideoWidgetStatus;
    friend class EmptyStatus;
    friend class LoadingStatus;
    friend class ReadyStatus;
    friend class PlayingStatus;
    friend class PauseStatus;
};