#include "videoWidget.hpp"

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QApplication>

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

    // 设置窗口属性
    setAcceptDrops(true); // 支持从文件夹拖拽
    setFocusPolicy(Qt::StrongFocus); // 支持快捷键输入
    setAttribute(Qt::WA_TranslucentBackground);
}

void VideoWidget::dragEnterEvent(QDragEnterEvent *event) {
    event->acceptProposedAction();

    QWidget::dragEnterEvent(event);
}

void VideoWidget::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();

    // 检查是否包含文件 URL
    if (mimeData->hasUrls()) {
        // 获取第一个文件 URL
        QUrl fileUrl = mimeData->urls()[0];
        
        // 转换为本地文件路径
        QString filePath = fileUrl.toLocalFile();
        
        playVideo(filePath);
    }

    QWidget::dropEvent(event);
}

void VideoWidget::resizeEvent(QResizeEvent* event) {
    vcanvas->resize(size());
}

bool VideoWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == vcanvas && event->type() == QEvent::Paint) {
        qDebug() << "[VideoWidget]: vcanvas paint";
        // update();
    }
    return QWidget::eventFilter(obj, event);
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

void VideoWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Left){
        setPosition(position() - skipStep);
    } else if(event->key() == Qt::Key_Right) {
        setPosition(position() + skipStep);
    } else if(event->key() == Qt::Key_Up) {
        qDebug() << "[videoWidget][volume ++][before] : \n" << "volume : " << volume() << "\n audio->volume : " << audio->volume() << "\n audio->volume * 100 : " << audio->volume() * 100.0;
        setVolume(volume() + 10);
        qDebug() << "[videoWidget][volume ++][after] : \n" << "volume : " << volume() << "\n audio->volume : " << audio->volume();
    } else if(event->key() == Qt::Key_Down) {
        qDebug() << "[videoWidget][volume --][befor] : \n" << "volume : " << volume() << "\n audio->volume : " << audio->volume() << "\n audio->volume * 100 : " << audio->volume() * 100.0;
        setVolume(volume() - 10);
        qDebug() << "[videoWidget][volume --][after] : \n" << "volume : " << volume() << "\n audio->volume : " << audio->volume();
    } else if (event->key() == Qt::Key_Space) {
        if (player->hasVideo()) {
            player->isPlaying() ? player->pause() : player->play();
        }
    }

    QWidget::keyPressEvent(event);
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