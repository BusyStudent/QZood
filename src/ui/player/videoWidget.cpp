#include "videoWidget.hpp"

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QShortcut>

VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(100,75);
    audio = new NekoAudioOutput();
    player = new NekoMediaPlayer();
    player->setAudioOutput(audio);
    vcanvas = new VideoCanvas(this);
    vcanvas->lower();
    vcanvas->attachPlayer(player);
    vcanvas->resize(size());
    vcanvas->installEventFilter(this);

    connect(player, &NekoMediaPlayer::positionChanged, this ,[this](double position){
        emit positionChanged(position);
    });

    connect(player, &NekoMediaPlayer::durationChanged, this, [this](double duration){
        emit durationChanged(duration);
    });

    connect(audio, &NekoAudioOutput::volumeChanged, this, [this](float value){
        emit volumeChanged((value + 0.005) * 100);
    });

    connect(player, &NekoMediaPlayer::errorOccurred, this, [this](NekoMediaPlayer::Error error, const QString &errorString){
        emit runError(errorString);
    });

    connect(player, &NekoMediaPlayer::seekableChanged, this, [this](bool v) {
        emit seekableChanged(v);
    });

    connect(player, &NekoMediaPlayer::sourceChanged, this, [this](const QUrl& url) {
        emit sourceChanged(url);
    });

    connect(player, &NekoMediaPlayer::bufferProgressChanged, this, [this](double sec){
        emit bufferProgressChanged(sec);
    });

    connect(player, &NekoMediaPlayer::playbackStateChanged, [this](NekoMediaPlayer::PlaybackState newState) {
        switch (newState)
        {
        case NekoMediaPlayer::PlaybackState::PlayingState:
            emit playing();
            break;
        case NekoMediaPlayer::PlaybackState::PausedState:
            emit paused();
            break;
        case NekoMediaPlayer::PlaybackState::StoppedState:
            emit stoped();
            break;
        default:
            qWarning() << "unknow player status occurred!";
        }
    });

    // 设置快捷键
    QShortcut* keySpace = new QShortcut(Qt::Key_Space, this);
    connect(keySpace, &QShortcut::activated, this, [this](){
        if (player->hasVideo()) {
            player->isPlaying() ? player->pause() : player->play();
        }
    });
    QShortcut* keyLeft = new QShortcut(Qt::Key_Left, this);
    connect(keyLeft, &QShortcut::activated, this, [this](){
        setPosition(position() - skipStep);
    });
    QShortcut* keyRight = new QShortcut(Qt::Key_Right, this);
    connect(keyRight, &QShortcut::activated, this, [this](){
        setPosition(position() + skipStep);
    });
    QShortcut* keyUp = new QShortcut(Qt::Key_Up, this);
    connect(keyUp, &QShortcut::activated, this, [this](){
        setVolume(volume() + 10);
    });
    QShortcut* keyDown = new QShortcut(Qt::Key_Down, this);
    connect(keyDown, &QShortcut::activated, this, [this](){
        setVolume(volume() - 10);
    });
}

void VideoWidget::resizeEvent(QResizeEvent* event) {
    vcanvas->resize(size());
}

void VideoWidget::setVolume(int value) {
    value = std::clamp(value, 0, 100);
    audio->setVolume((float) value / 100.0);
}

int VideoWidget::duration() {
    if (!player->hasVideo()) {
        emit runError("请先播放视频");
        return 0;
    }
    return player->duration();
}

void VideoWidget::setPosition(int sec) {
    if (!player->hasVideo()) {
        emit runError("请先播放视频");
        return;
    }
    player->setPosition(sec);

    // TODO(llhsdmd@gmail.com,BusyStudent): 增加弹幕是否装载的判定
    // vcanvas->setDanmakuPosition(sec);

    // TODO(BusyStudent): 字幕同弹幕一样
}

void VideoWidget::setSkipStep(int sec) {
    skipStep = sec;
}

void VideoWidget::playVideo(const QUrl& url) {
    player->setSource(url);

    player->play();
    emit playing();
}

void VideoWidget::playVideo(QIODevice* device) {
    player->setSourceDevice(device);
    player->play();
}

void VideoWidget::pauseVideo() {
    if (!player->hasVideo()) {
        emit runError("请先播放视频");
        return;
    }
    player->pause();
}

void VideoWidget::resumeVide() {
    if (!player->hasVideo()) {
        emit runError("请先播放视频");
        return;
    }
    player->play();
}

int VideoWidget::position() {
    if (!player->hasVideo()) {
        emit runError("请先播放视频");
        return 0;
    }
    return player->position();
}

VideoWidget::~VideoWidget() {
    if (player->isPlaying()) {
        player->stop();
    }
    delete audio;
    delete player;
}