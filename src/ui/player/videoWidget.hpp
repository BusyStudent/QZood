#pragma once

#include <QWidget>

#include "../../player/videocanvas.hpp"

class VideoWidget : public QWidget{
    Q_OBJECT
    public:
        VideoWidget(QWidget* parent = nullptr);
        ~VideoWidget();
        int duration();
        int position();
        inline int volume() { return (audio->volume() + 0.005) * 100; }
        inline bool isSeekable() { return player->isSeekable(); }

    public:
        void resizeEvent(QResizeEvent* event) override;
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        bool eventFilter(QObject* obj, QEvent* event) override;

    Q_SIGNALS:
        void sourceChanged(const QUrl& url);

        void durationChanged(int sec);

        void seekableChanged(bool seekable);

        void positionChanged(int sec);
        void bufferProgressChanged(int sec);

        void volumeChanged(int value);

        void runError(const QString& errorMessage);
        
        void playing();
        void paused();
        void stoped();

        void skipStepChange(int sec);

    public Q_SLOTS:
        void setVolume(int value);
        void setPosition(int sec);
        void setSkipStep(int sec);
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

        int skipStep = 5;
};