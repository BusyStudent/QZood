#pragma once

#include <QWidget>

#include "../../BLL/data/videoBLL.hpp"

class VideoWidgetPrivate;

enum Order {
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
        VideoWidgetPrivate* d;
};