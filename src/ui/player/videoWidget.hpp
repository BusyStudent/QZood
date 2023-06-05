#pragma once

#include <QWidget>

#include "../../player/videocanvas.hpp"

class VideoWidget : public QWidget{
    Q_OBJECT
    public:
        VideoWidget(QWidget* parent = nullptr);
        int duration();
        int position();
        int volume();

    public:
        void resizeEvent(QResizeEvent* event) override;

    Q_SIGNALS:
        void positionChanged(int sec);
        void durationChanged(int sec);
        void volumeChanged(int value);

    public Q_SLOTS:
        void setVolume(int value);
        void setPosition(int sec);
        void playVideo(const QUrl& url);
        void playVideo(QIODevice* device);
        void pauseVideo();
        void resumeVide();
        // void setDanmaku(int id);
        // void setSubtitle();

    private:
        VideoCanvas* vcanvas;
        NekoMediaPlayer* player;
        NekoAudioOutput* audio;
};