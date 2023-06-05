#include "videoWidget.hpp"

#include <QEvent>

VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent) {
    audio = new NekoAudioOutput(this);
    player = new NekoMediaPlayer(this);
    player->setAudioOutput(audio);
    vcanvas = new VideoCanvas(this);
    vcanvas->attachPlayer(player);
    vcanvas->lower();
    vcanvas->resize(size());
    vcanvas->installEventFilter(this);

    connect(player, &NekoMediaPlayer::positionChanged, this ,[this](double position){
        emit positionChanged(position);
    });

    connect(player, &NekoMediaPlayer::durationChanged, this, [this](double duration){
        emit durationChanged(duration);
    });

    connect(audio, &NekoAudioOutput::volumeChanged, this, [this](float value){
        emit volumeChanged(value * 100);
    });
}

void VideoWidget::resizeEvent(QResizeEvent* event) {
    vcanvas->resize(size());
}

int VideoWidget::volume() {
    return audio->volume() * 100;
}

void VideoWidget::setVolume(int value) {
    audio->setVolume((float) value / 100.0);
}

int VideoWidget::duration() {
    return player->duration();
}

void VideoWidget::setPosition(int sec) {
    player->setPosition(sec);
    vcanvas->setDanmakuPosition(sec);
}

void VideoWidget::playVideo(const QUrl& url) {
    player->setSource(url);
    player->play();
}

void VideoWidget::playVideo(QIODevice* device) {
    player->setSourceDevice(device);
    player->play();
}

void VideoWidget::pauseVideo() {
    player->pause();
}

void VideoWidget::resumeVide() {
    player->play();
}

int VideoWidget::position() {
    return player->position();
}