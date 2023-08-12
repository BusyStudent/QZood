#include "videoWidget.hpp"
#include "ui_videoSettingView.h"
#include "ui_SourcesListContainer.h"

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QShortcut>
#include <QPushButton>

#include "../../BLL/data/videoItemModel.hpp"
#include "../util/widget/popupWidget.hpp"
#include "volumeSettingWidget.hpp"
#include "videoWidget.hpp"
#include "fullSettingWidget.hpp"
#include "../util/widget/customSlider.hpp"
#include "videoWidgetStatus.hpp"
#include "../../common/myGlobalLog.hpp"

#include "settings/playSetting.hpp"
#include "settings/screenSetting.hpp"
#include "settings/colorSetting.hpp"
#include "settings/danmakuSetting.hpp"
#include "settings/subtitleSetting.hpp"

#include "logWidget.hpp"

static QString timeFormat(int sec) {
    if (sec < 60 * 60) {
        return QString("%1:%2").
            arg(sec / 60, 2, 10, QLatin1Char('0')).
            arg(sec % 60, 2, 10, QLatin1Char('0'));
    } else {
        return QString("%1:%2:%3").
            arg(sec / 3600).
            arg((sec % 3600) / 60, 2, 10, QLatin1Char('0')).
            arg(sec % 60, 2, 10, QLatin1Char('0'));
    }
}

class VideoWidgetPrivate {
public:
    VideoWidgetPrivate(VideoWidget* parent) : self(parent) {
        mMainLayout = new QVBoxLayout(self);
        self->setMouseTracking(true);
    }

    ~VideoWidgetPrivate() {
        if (mPlayer->isPlaying()) {
            mPlayer->stop();
        }
        delete mAudio;
        delete mPlayer;
    }

    void setupUi() {
        // 构建核心播放器界面
        mAudio = new NekoAudioOutput();
        mPlayer = new NekoMediaPlayer();
        mPlayer->setAudioOutput(mAudio);
        mVcanvas = new VideoCanvas(self);
        mVcanvas->lower();
        mVcanvas->attachPlayer(mPlayer);
        mVcanvas->setAttribute(Qt::WA_TransparentForMouseEvents, true);;

        mMainLayout->addStretch();
        mMainLayout->setContentsMargins(1, 0, 1, 0);

        // 实例化视频播放进度条及设置栏
        mVideoSetting = new PopupWidget();
        ui_videoSetting.reset(new Ui::VideoSettingView());
        ui_videoSetting->setupUi(mVideoSetting);
        mMainLayout->addWidget(mVideoSetting);
        ui_videoSetting->playerButton->setCheckable(false);
        ui_videoSetting->playerButton->setChecked(false);
        mVideoSetting->show();

        // 实例化视频进度条
        mVideoProgressBar = new CustomSlider();
        mVideoProgressBar->setObjectName("videoProgressBar");
        static_cast<QVBoxLayout*>(mVideoSetting->layout())->insertWidget(0, mVideoProgressBar);

        // 实例化音量控制界面
        mVolumeSetting = new VolumeSettingWidget(self);
        mVolumeSetting->setObjectName("VolumeSettingWidget");
        ui_videoSetting->voiceSettingButton->installEventFilter(self);
        mVolumeSetting->setAssociateWidget(ui_videoSetting->voiceSettingButton);
        mVolumeSetting->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        mVolumeSetting->setAuotLayout();
        mVolumeSetting->setOutside(true);
        mVolumeSetting->setHideAfterLeave(true);

        // 播放设置控件
        mSettings = new FullSettingWidget(self, Qt::Popup | Qt::WindowStaysOnTopHint);
        mSettings->setObjectName("FullSettingWidget");
        mSettings->setAssociateWidget(ui_videoSetting->settingButton);
        mSettings->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        mSettings->setAuotLayout();
        mSettings->setHideAfterLeave(false);
        mSettings->setOutside(true);

        // 显示信息的标签
        mLogWidget = new LogWidget(self);
        mLogWidget->setAssociateWidget(mVideoSetting);
        mLogWidget->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        mLogWidget->setAuotLayout(true);
        mLogWidget->setHideAfterLeave(false);
        mLogWidget->setStopTimerEnter(false);
        mLogWidget->hide();
        mLogWidget->setOutside(true);

        // 源列表
        mSourceListWidget = new PopupWidget(self);
        ui_sourceList.reset(new Ui::SourceList());
        ui_sourceList->setupUi(mSourceListWidget);
        mSourceListWidget->setAssociateWidget(ui_videoSetting->sourceButton);
        mSourceListWidget->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        mSourceListWidget->setAuotLayout();
        mSourceListWidget->setOutside(true);
        
        // 设置设置界面
        mSettings->addSettingItem(new PlaySetting(mSettings), self);
        mSettings->addSettingItem(new ScreenSetting(mSettings), self);
        mSettings->addSettingItem(new ColorSetting(mSettings), self);
        mSettings->addSettingItem(new DanmakuSetting(mSettings), self);
        mSettings->addSettingItem(new SubtitleSetting(mSettings), self);
    }

    void setupShortcut() {
        // 设置快捷键
        QShortcut* keySpace = new QShortcut(Qt::Key_Space, self);
        QWidget::connect(keySpace, &QShortcut::activated, self, [this](){
            if (mPlayer->isLoaded()) {
                mPlayer->isPlaying() ? pause() : resume();
            }
        });
        QShortcut* keyLeft = new QShortcut(Qt::Key_Left, self);
        QWidget::connect(keyLeft, &QShortcut::activated, self, [this](){
            videoLog(QString("快退 %1s").arg(mSkipStep));
            setPosition(position() - mSkipStep);
        });
        QShortcut* keyRight = new QShortcut(Qt::Key_Right, self);
        QWidget::connect(keyRight, &QShortcut::activated, self, [this](){
            videoLog(QString("快进 %1s").arg(mSkipStep));
            setPosition(position() + mSkipStep);
        });
        QShortcut* keyUp = new QShortcut(Qt::Key_Up, self);
        QWidget::connect(keyUp, &QShortcut::activated, self, [this](){
            setVolume(volume() + 10);
            videoLog(QString("音量 %1").arg(volume()));
        });
        QShortcut* keyDown = new QShortcut(Qt::Key_Down, self);
        QWidget::connect(keyDown, &QShortcut::activated, self, [this](){
            setVolume(volume() - 10);
            videoLog(QString("音量 %1").arg(volume()));
        });
        QShortcut* keyEsc = new QShortcut(Qt::Key_Escape, self);
        QWidget::connect(keyEsc, &QShortcut::activated, self, [this](){
            qDebug() << "keyEsc";
            if (self->isFullScreen()) {
                ui_videoSetting->showFullScreenButton->click();
            }
        });
    }

    void connect() {
        connectVideoProgressBar();
        connectVolumeSetting();
        connectVideoPlayBar();
        connectVideoPlaySetting();
        connectVideoStatus();
        connectSourceList();
    }

    void update() {
        // 进度条
        mVideoProgressBar->setValue(position());
        mVolumeSetting->setValue(volume());
    }

    void videoLog(const QString& msg) {
        mLogWidget->setDefualtHideTime(2000);
        mLogWidget->pushLog(msg);
    }

    void pause() {
        if (mPlayer->isPlaying()) {
            savePlayStatus();
            mPlayer->pause();
        } else {
            videoLog("请先播放视频");
        }
    }
    
    void deleteAllChildren(QWidget* widget) {
        for (auto item : widget->findChildren<QWidget*>()) {
            item->setParent(nullptr);
            item->deleteLater();
        }
    }

    void updateSourceList(const QStringList& sourceList) {
        deleteAllChildren(mSourceListWidget);
        for (auto source : sourceList) {
            QPushButton *button = new QPushButton(mSourceListWidget);
            mSourceListWidget->layout()->addWidget(button);
            button->setText(source);
            QWidget::connect(button, &QPushButton::clicked, self, [this, source](bool checked){
                if (mVideo->getCurrentVideoSource() != source) {
                    self->status()->changeSource(source);
                }
            });
        }
        mSourceListWidget->resize(mSourceListWidget->sizeHint());
    }

    void setVideoSource(const QString &source) {
        stop();
        videoLog("正在切换视频源");
        mVideo->setCurrentVideoSource(source);
        ui_videoSetting->sourceButton->setText(source);
        videoLog("切换完成");
    }

    void resume() {
        if (mPlayer->hasVideo() && !mPlayer->isPlaying()) {
            mPlayer->play();
        } else {
            videoLog("没有视频暂停");
        }
    }

    int volume() {
        return (mAudio->volume() + 0.005) * 100;
    }

    void setVolume(int value) {
        value = std::clamp(value, 0, 100);
        mAudio->setVolume((float) value / 100.0);
    }

    int duration() {
        if (!mPlayer->hasVideo()) {
            return 0;
        }
        return mPlayer->duration();
    }

    void setPosition(int sec) {
        if (!mPlayer->hasVideo()) {
            return;
        }
        mPlayer->setPosition(sec);
        mVcanvas->setDanmakuPosition(sec);
    }

    void setSkipStep(int sec) {
        mSkipStep = sec;
    }
    
    int position() {
        if (!mPlayer->hasVideo()) {
            return 0;
        }
        return mPlayer->position();
    }

    void stop() {
        if (mPlayer->isPlaying()) {
            savePlayStatus();
            mPlayer->stop();
        }
    }
    
    void clean() {
        stop();
        deleteAllChildren(mSourceListWidget);
        mVideoProgressBar->setValue(0);
        mVideoProgressBar->setPreloadValue(0);
    }

    void hideCursor() {
        self->setCursor(Qt::BlankCursor);
    }

    void showCursor() {
        if (self->cursor() != cursorShape) {
            self->setCursor(cursorShape);
        }
    }
private:
     /**
     * @brief 视频进度条
     */
    void connectVideoProgressBar() {
        mVideoProgressBar->setDisabled(true);
        // 处理来自进度条的请求
        QWidget::connect(mVideoProgressBar, &CustomSlider::sliderMoved, self, [this](int position){
            if (mPlayer->isSeekable()) {
                self->status()->changePostion(position);
            }
        });

        // 同步更新进度条的进度信息
        QWidget::connect(mPlayer, &NekoMediaPlayer::durationChanged, self, [this](qreal sec){
            mVideoProgressBar->setRange(0, sec);
        });

        QWidget::connect(mPlayer, &NekoMediaPlayer::seekableChanged, self, [this](bool v) {
            mVideoProgressBar->setEnabled(v);
        });

        QWidget::connect(mPlayer, &NekoMediaPlayer::positionChanged, self, [this](int value){
            auto total = timeFormat(mPlayer->duration());
            auto current = timeFormat(value);
            if (total.length() > current.length()) {
                current = QString("%1:%2").
                    arg(0,total.length() - current.length() - 1, 10, QLatin1Char('0')).
                    arg(current);
            }
            ui_videoSetting->videoTimeLabel->setText(current + "/" + total);
            if (mVideoProgressBar->isSliderDown()) {
                return;
            }
            mVideoProgressBar->setValue(value);
        });

        QWidget::connect(mVideoProgressBar, &CustomSlider::tipBeforeShow, self, [this](QLabel *tipLabel, int value){
            tipLabel->setText(timeFormat(value));
        });

        QWidget::connect(mPlayer, &NekoMediaPlayer::bufferedDurationChanged, self, [this](double sec){
            mVideoProgressBar->setPreloadValue(sec + position());
        });
        // 结束信号
        QWidget::connect(mPlayer, &NekoMediaPlayer::errorOccurred, self, [this](NekoMediaPlayer::Error error, const QString &errorString){
            videoLog(errorString);
            // Q_EMIT self->finished();
        });
        QWidget::connect(mPlayer, &NekoMediaPlayer::mediaStatusChanged, self, [this](NekoMediaPlayer::MediaStatus status){
            switch (status) {
            case NekoMediaPlayer::MediaStatus::InvalidMedia:
                videoLog("非法文件");
                Q_EMIT self->invalidVideo(mVideo);
                break;
            case NekoMediaPlayer::MediaStatus::EndOfMedia:
                Q_EMIT self->finished();
                break;
            default:
                break;
            }
        });
    }

    /**
     * @brief 音量设置界面
     */
    void connectVolumeSetting() {
        // 连接音量调节条显示：定义弹出窗口显示时进度条不自动隐藏。
        QWidget::connect(mVolumeSetting, &VolumeSettingWidget::showed, self, [this](){
            mVideoSetting->setHideAfterLeave(false);
            mVideoSetting->show();
            mVideoSetting->stopHideTimer();
        });
        // 连接音量调节条隐藏：恢复视频进度条自动隐藏
        QWidget::connect(mVolumeSetting, &VolumeSettingWidget::hided, self, [this](){
            mVideoSetting->setHideAfterLeave(true);
            mVideoSetting->setDefualtHideTime(100);
            mVideoSetting->hideLater(5000);
        });
        // 按钮图标并更新程序播放音量
        QWidget::connect(mAudio, &NekoAudioOutput::volumeChanged, self, [this](float value){
            mVolumeSetting->setValue((value + 0.005) * 100);
        });
        // 连接音量调节条调节：同步音量
        QWidget::connect(mVolumeSetting, &VolumeSettingWidget::sliderMoved, self, [this](int value) {
            mAudio->setVolume(value / 100.0);
        });
        // 连接视频播放进度条隐藏：隐藏自己
        QWidget::connect(mVideoSetting, &PopupWidget::hided, mVolumeSetting, &VolumeSettingWidget::hide);
        // 连接音量按钮点击：切换静音-有声模式
        QWidget::connect(ui_videoSetting->voiceSettingButton, &QToolButton::clicked, self,  [this, volume = 0.0]() mutable {
            if (ui_videoSetting->voiceSettingButton->isChecked()) {
                volume = mAudio->volume();
                mAudio->setVolume(0);
            } else {
                mAudio->setVolume(volume);
            }
        });
        // 同步音量按钮图标
        QWidget::connect(mVolumeSetting, &VolumeSettingWidget::valueChanged, self, [this, flag = -1](int value) mutable {
            if (value == 0 && flag != 0) {
                flag = 0;
                QIcon icon;
                icon.addFile(QString::fromUtf8(":/icons/mute_white.png"), QSize(), QIcon::Normal, QIcon::Off);
                ui_videoSetting->voiceSettingButton->setIcon(icon);
            } else if (value > 0 && value <= 33 && flag != 1) {
                flag = 1;
                QIcon icon;
                icon.addFile(QString::fromUtf8(":/icons/volume_1_white.png"), QSize(), QIcon::Normal, QIcon::Off);
                ui_videoSetting->voiceSettingButton->setIcon(icon);
            } else if (value > 33 && value <= 66 && flag != 2) {
                flag = 2;
                QIcon icon;
                icon.addFile(QString::fromUtf8(":/icons/volume_2_white.png"), QSize(), QIcon::Normal, QIcon::Off);
                ui_videoSetting->voiceSettingButton->setIcon(icon);
            } else if (value > 66 && flag != 3){
                flag = 3;
                QIcon icon;
                icon.addFile(QString::fromUtf8(":/icons/volume_3_white.png"), QSize(), QIcon::Normal, QIcon::Off);
                ui_videoSetting->voiceSettingButton->setIcon(icon);
            }
        });
    }

     /**
     * @brief 视频播放设置栏
     * 
     */
    void connectVideoPlayBar() {
        // 连接播放按钮的行为
        QWidget::connect(ui_videoSetting->playerButton, &QToolButton::clicked, self, [this](bool checked){
            if (!checked) {
                self->status()->pause();
            } else {
                self->status()->play();
            }
        });
        QWidget::connect(mPlayer, &NekoMediaPlayer::playbackStateChanged, self, [this](NekoMediaPlayer::PlaybackState status){
            switch (status){
                case NekoMediaPlayer::PlayingState:
                    ui_videoSetting->playerButton->setCheckable(true);
                    ui_videoSetting->playerButton->setChecked(true);
                    emit self->playing();
                    break;
                case NekoMediaPlayer::PausedState:
                    ui_videoSetting->playerButton->setCheckable(true);
                    ui_videoSetting->playerButton->setChecked(false);
                    emit self->paused();
                    break;
                case NekoMediaPlayer::StoppedState:
                    ui_videoSetting->playerButton->setCheckable(false);
                    ui_videoSetting->playerButton->setChecked(false);
                    emit self->stoped();
                    break;
            }
        });
        // 切集播放按钮
        QWidget::connect(ui_videoSetting->FowardButton, &QToolButton::clicked, self, [this](bool clicked){
            emit self->nextVideo();
        });
        QWidget::connect(ui_videoSetting->BackwardButton, &QToolButton::clicked, self, [this](bool clicked){
            emit self->previousVideo();
        });
        QWidget::connect(ui_videoSetting->showFullScreenButton, &QToolButton::clicked, self, [this, parent = self->parentWidget()](bool clicked) mutable {
            if (!self->isFullScreen()) {
                self->setWindowFlags(Qt::Window);
                mSettings->setParent(self, Qt::Widget);
                mSettings->raise();
                self->showFullScreen();
            } else {
                self->setWindowFlags(Qt::SubWindow);
                mSettings->setParent(self, Qt::Popup);
                self->showNormal();
            }
        });
    }

    /**
     * @brief 播放状态恢复
     * 
     */
    void connectVideoStatus() {
        QWidget::connect(mPlayer, &NekoMediaPlayer::mediaStatusChanged, self, [this](NekoMediaPlayer::MediaStatus status){
            switch (status) {
            case NekoMediaPlayer::MediaStatus::LoadedMedia:
                resumePlayStatus();
                break;
            }
        });
    }

    /**
     * @brief 视频播放设置
     * 
     */
    void connectVideoPlaySetting() {
        // 设置窗口显示和隐藏
        QWidget::connect(mVideoSetting, &PopupWidget::hided,self, [this](){
            mSettings->hide();
            hideCursor();
        });
        QWidget::connect(mSettings, &FullSettingWidget::showed, self, [this](){
            mVideoSetting->show();
            mVideoSetting->setDefualtHideTime(std::numeric_limits<int>::max());
        });
        QWidget::connect(mSettings, &FullSettingWidget::hided, self, [this](){
            mVideoSetting->setDefualtHideTime(100);
            mVideoSetting->hideLater(5000);
        });
        QWidget::connect(ui_videoSetting->settingButton, &QToolButton::clicked, mSettings, [this](){
            // TODO(llhsdmd):popup属性的窗口在全屏模式下无法正常的弹出。
            if (mPlayer->isLoaded()) {
                if (mSettings->isHidden()) {
                    mSettings->show();
                    mSettings->hideLater(5000);
                } else {
                    mSettings->hide();
                }
            }
        });
    }

    void connectSourceList() {
        QWidget::connect(ui_videoSetting->sourceButton, &QToolButton::clicked, mSourceListWidget, [this](bool checked) {
            mVideoSetting->setHideAfterLeave(false);
            mSourceListWidget->show();
            mSourceListWidget->hideLater(5000);
        });
        QWidget::connect(mSourceListWidget, &PopupWidget::hided, self, [this]() {
            mVideoSetting->setHideAfterLeave(true);
            mVideoSetting->hideLater();
        });
    }

    void savePlayStatus() {
        mVideo->setStatus("position", position());
    }

    void resumePlayStatus() {
        if (mVideo->containsStatus("position")) {
            setPosition(mVideo->getStatus<int>("position").value_or(0));
        }
        qWarning() << "resumePlayStatus";
        for (const auto source : mPlayer->subtitleTracks()) {
            qWarning() << "subtitle title : " << source[NekoMediaMetaData::Title];
            mVideo->addSubtitleSource(source[NekoMediaMetaData::Title]);
        }
        mSettings->refresh();
    }

public:
    VideoCanvas *mVcanvas;
    NekoMediaPlayer *mPlayer;
    NekoAudioOutput *mAudio;

    PopupWidget *mVideoSetting = nullptr;
    QScopedPointer<Ui::VideoSettingView> ui_videoSetting;
    CustomSlider *mVideoProgressBar = nullptr;

    VolumeSettingWidget *mVolumeSetting = nullptr;

    PopupWidget *mSourceListWidget = nullptr;
    QScopedPointer<Ui::SourceList> ui_sourceList;

    FullSettingWidget *mSettings = nullptr;

    LogWidget *mLogWidget = nullptr;

    VideoBLLPtr mVideo;

    int mSkipStep = 10;
    bool mIsLoddingVideo = false;

private:
    VideoWidget *self;
    QVBoxLayout *mMainLayout;
    Qt::CursorShape cursorShape = Qt::CursorShape::ArrowCursor;
};

VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent), d(new VideoWidgetPrivate(this)) {
    d->setupUi();
    d->connect();
    d->setupShortcut();

    setMinimumSize(100,75);

    d->update();
    changeStatus(new EmptyStatus(this));
}

void VideoWidget::resizeEvent(QResizeEvent* event) {
    videoCanvas()->resize(size());
}

VideoCanvas* VideoWidget::videoCanvas() {
    return d->mVcanvas;
}

NekoMediaPlayer* VideoWidget::player() {
    return d->mPlayer;
}

VideoWidgetStatus* VideoWidget::status() {
    return mStatus.get();
}

bool VideoWidget::event(QEvent *event) {
  if (event->type() == QEvent::HoverMove) {
    QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
    QMouseEvent mouseEvent(QEvent::MouseMove, hoverEvent->pos(), Qt::NoButton,
                           Qt::NoButton, Qt::NoModifier);

    mouseMoveEvent(&mouseEvent);
  }

  return QWidget::event(event);
}

bool VideoWidget::eventFilter(QObject *obj, QEvent *event) {
    if (d->mVideoSetting != nullptr && obj == d->ui_videoSetting->voiceSettingButton) {
        if (event->type() == QEvent::Enter) {
            d->mVolumeSetting->setDefualtHideTime(5000);
            d->mVolumeSetting->show();
            d->mVolumeSetting->setFocus();
        } else if (event->type() == QEvent::Leave) {
            d->mVolumeSetting->hideLater(300);
        }
    }

    return QWidget::eventFilter(obj, event);
}

void VideoWidget::leaveEvent(QEvent* event) {
    d->mVideoSetting->hideLater();
    QWidget::leaveEvent(event);
}

void VideoWidget::mouseMoveEvent(QMouseEvent* event) {
    QMetaObject::invokeMethod(this, [this](){
            d->mVideoSetting->show();
            d->mVideoSetting->hideLater(5000);
            d->showCursor();
        }, Qt::QueuedConnection);

    QWidget::mouseMoveEvent(event);
}
void VideoWidget::playVideo(const VideoBLLPtr video) {
    status()->LoadVideo(video);
    status()->play();
}
void VideoWidget::stop() {
    d->stop();
}
void VideoWidget::changeStatus(VideoWidgetStatus* status) {
    mStatus.reset(status);
    MDebug(MyDebug::INFO) << "video widget status : " << status->type();
}
void VideoWidget::videoLog(const QString& info) {
    d->videoLog(info);
}
int VideoWidget::skipStep() {
    return d->mSkipStep;
}
void VideoWidget::setSkipStep(int v) {
    d->setSkipStep(v);
}
void VideoWidget::setPlaybackRate(qreal v) {
    d->mPlayer->setPlaybackRate(v);
}
// 当前指针
VideoBLLPtr VideoWidget::currentVideo() {
    return d->mVideo;
}

VideoWidget::~VideoWidget() {
    status()->clean();
}

VideoWidgetStatus::VideoWidgetStatus(VideoWidget* self) : self(self) { }

int VideoWidgetStatus::clean() {
    self->d->clean();
    self->changeStatus(new EmptyStatus(self));
    return OK;
}

int VideoWidgetStatus::changeSource(const QString &source) {
    self->d->setVideoSource(source);
    auto status = new ReadyStatus(self);
    self->changeStatus(status);
    status->play();
    return OK;
}

int VideoWidgetStatus::changePostion(int pos) {
    self->d->setPosition(pos);
    return OK;
}

EmptyStatus::EmptyStatus(VideoWidget* self) : VideoWidgetStatus(self) { 
    self->player()->setHttpReferer("https://www.bilibili.com");
    self->player()->setHttpUseragent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537");
    self->player()->setOption("multiple_requests", "1");
}

QString EmptyStatus::type() {
    return "EmptyStatus";
}

int EmptyStatus::LoadVideo(const VideoBLLPtr video) {
    self->d->updateSourceList(video->sourcesList());
    self->d->ui_videoSetting->sourceButton->setText(video->getCurrentVideoSource());
    self->d->mVideo = video;
    self->changeStatus(new ReadyStatus(self));
    return OK;
}

int EmptyStatus::changeSource(const QString &source) {
    return NO_VIDEO;
}

int EmptyStatus::changePostion(int pos) {
    return INVALID;
}

int EmptyStatus::play() {
    self->videoLog("请先加载视频");
    return NO_VIDEO;
}

int EmptyStatus::pause() {
    self->videoLog("请先加载视频");
    return NO_VIDEO;
}

int EmptyStatus::clean() { 
    return NO_VIDEO;
 }

LoadingStatus::LoadingStatus(VideoWidget* self) : VideoWidgetStatus(self) { }

QString LoadingStatus::type() {
    return "LoadingStatus";
}

int LoadingStatus::LoadVideo(const VideoBLLPtr video) {
    self->videoLog("有视频正在加载中，无法进行该操作。");
    return VIDEO_LOADING;
}

int LoadingStatus::changeSource(const QString &source) { 
    return VIDEO_LOADING;
 }

int LoadingStatus::changePostion(int pos) {
    return INVALID;
}

int LoadingStatus::play() {
    self->videoLog("视频加载中...");
    return VIDEO_LOADING;
}

int LoadingStatus::pause() {
    self->videoLog("视频加载中...");
    return VIDEO_LOADING;
}

int LoadingStatus::clean() {
    self->videoLog("有视频正在加载中，无法进行该操作。");
    return VIDEO_LOADING;
}

ReadyStatus::ReadyStatus(VideoWidget* self) : VideoWidgetStatus(self) { }

QString ReadyStatus::type() {
    return "ReadyStatus";
}

int ReadyStatus::LoadVideo(const VideoBLLPtr video) {
    self->d->clean();
    auto status = new EmptyStatus(self);
    self->changeStatus(status);
    status->LoadVideo(video);
    return OK;
}

int ReadyStatus::changePostion(int pos) {
    return INVALID;
}

int ReadyStatus::play() {
    self->videoLog("开始加载视频");
    self->currentVideo()->loadVideoToPlay(self, [self = this->self](const Result<QString>& url) {
        if (url.has_value()) {
            self->player()->setSource(url.value());
            self->player()->play();
            self->videoLog("视频加载完成");
            self->videoLog("视频开始播放");
            self->changeStatus(new PlayingStatus(self));
        } else {
            self->videoLog("视频加载失败");
            self->d->clean();
            self->changeStatus(new EmptyStatus(self));
        }
    });
    return OK;
}

int ReadyStatus::pause() {
    return OK;
}

int ReadyStatus::clean() {
    self->d->clean();
    return OK;
}

PlayingStatus::PlayingStatus(VideoWidget* self) : VideoWidgetStatus(self) { }

QString PlayingStatus::type() {
    return "PlayingStatus";
}

int PlayingStatus::LoadVideo(const VideoBLLPtr video) {
    self->d->clean();
    auto status = new EmptyStatus(self);
    self->changeStatus(status);
    status->LoadVideo(video);
    return OK;
}

int PlayingStatus::play() {
    return INVALID;
}

int PlayingStatus::pause() {
    self->d->pause();
    self->changeStatus(new PauseStatus(self));
    return OK;
}

int PlayingStatus::clean() {
    self->d->clean();
    self->changeStatus(new EmptyStatus(self));
    return OK;
}

PauseStatus::PauseStatus(VideoWidget* self) : VideoWidgetStatus(self) { }

QString PauseStatus::type()
{
    return "PauseStatus";
}

int PauseStatus::LoadVideo(const VideoBLLPtr video) {
    self->d->clean();
    auto status = new EmptyStatus(self);
    self->changeStatus(status);
    status->LoadVideo(video);
    return OK;
}

int PauseStatus::play() {
    self->d->resume();
    self->changeStatus(new PlayingStatus(self));
    return OK;
}

int PauseStatus::pause() {
    return INVALID;
}

int PauseStatus::clean() {
    self->d->clean();
    self->changeStatus(new EmptyStatus(self));
    return OK;
}