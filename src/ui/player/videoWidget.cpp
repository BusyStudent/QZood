#include "videoWidget.hpp"
#include "ui_videoSettingView.h"
#include "ui_customLabel.h"

#include <QEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QShortcut>

#include "../../BLL/data/videoItemModel.hpp"
#include "../common/popupWidget.hpp"
#include "volumeSettingWidget.hpp"
#include "videoWidget.hpp"
#include "fullSettingWidget.hpp"
#include "../common/customSlider.hpp"
#include "../../player/videocanvas.hpp"

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
        mainLayout = new QVBoxLayout(self);
        
    }

    ~VideoWidgetPrivate() {
        delete ui_videoSetting;
        if (player->isPlaying()) {
            player->stop();
        }
        delete audio;
        delete player;
    }

    void setupUi() {
        // 构建核心播放器界面
        audio = new NekoAudioOutput();
        player = new NekoMediaPlayer();
        player->setAudioOutput(audio);
        vcanvas = new VideoCanvas(self);
        vcanvas->lower();
        vcanvas->attachPlayer(player);

        mainLayout->addStretch();
        mainLayout->setContentsMargins(1, 0, 1, 0);

        // 实例化视频播放进度条及设置栏
        videoSetting = new PopupWidget();
        ui_videoSetting = new Ui::VideoSettingView();
        ui_videoSetting->setupUi(videoSetting);
        mainLayout->addWidget(videoSetting);
        ui_videoSetting->playerButton->setCheckable(false);
        ui_videoSetting->playerButton->setChecked(false);
        videoSetting->show();

        // 实例化视频进度条
        videoProgressBar = new CustomSlider();
        videoProgressBar->setObjectName("videoProgressBar");
        static_cast<QVBoxLayout*>(videoSetting->layout())->insertWidget(0, videoProgressBar);

        // 实例化音量控制界面
        volumeSetting = new VolumeSettingWidget(self);
        ui_videoSetting->voiceSettingButton->installEventFilter(self);
        volumeSetting->setAssociateWidget(ui_videoSetting->voiceSettingButton);
        volumeSetting->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        volumeSetting->setAuotLayout();
        // 播放设置控件
        settings = new FullSettingWidget(self, Qt::Popup);
        settings->setAssociateWidget(ui_videoSetting->settingButton);
        settings->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
        settings->setAuotLayout();
        settings->setHideAfterLeave(false);

        // 显示信息的标签
        // TODO(llhsdmd): 添加信息显示标签
    }
    
    void setupShortcut() {
        // 设置快捷键
        QShortcut* keySpace = new QShortcut(Qt::Key_Space, self);
        QWidget::connect(keySpace, &QShortcut::activated, self, [this](){
            if (player->hasVideo()) {
                player->isPlaying() ? pause() : resume();
            }
        });
        QShortcut* keyLeft = new QShortcut(Qt::Key_Left, self);
        QWidget::connect(keyLeft, &QShortcut::activated, self, [this](){
            setPosition(position() - skipStep);
        });
        QShortcut* keyRight = new QShortcut(Qt::Key_Right, self);
        QWidget::connect(keyRight, &QShortcut::activated, self, [this](){
            setPosition(position() + skipStep);
        });
        QShortcut* keyUp = new QShortcut(Qt::Key_Up, self);
        QWidget::connect(keyUp, &QShortcut::activated, self, [this](){
            setVolume(volume() + 10);
        });
        QShortcut* keyDown = new QShortcut(Qt::Key_Down, self);
        QWidget::connect(keyDown, &QShortcut::activated, self, [this](){
            setVolume(volume() - 10);
        });
    }

    void connect() {
        connectVideoProgressBar();
        connectVolumeSetting();
        connectVideoPlayBar();
        connectVideoPlaySetting();
        connectVideoStatus();
    }

    void update() {
        // 进度条
        videoProgressBar->setValue(position());
        volumeSetting->setValue(volume());
    }

    void videoLog(const QString& msg) {

    }

    void pause() {
        if (player->isPlaying()) {
            player->pause();
        } else {
            videoLog("请先播放视频");
        }
    }

    void play(const VideoBLLPtr video) {
        stop();
        this->video = video;
        video->loadVideoToPlay(self, [this](const Result<QString>& url){
            if (url.has_value()) {
                player->setSource(url.value());
                player->play();
            }
        });
    }

    void resume() {
        if (player->hasVideo() && !player->isPlaying()) {
            player->play();
        } else {
            videoLog("没有视频暂停");
        }
    }

    int volume() {
        return (audio->volume() + 0.005) * 100;
    }

    void setVolume(int value) {
        value = std::clamp(value, 0, 100);
        audio->setVolume((float) value / 100.0);
    }

    int duration() {
        if (!player->hasVideo()) {
            videoLog("请先播放视频");
            return 0;
        }
        return player->duration();
    }

    void setPosition(int sec) {
        if (!player->hasVideo()) {
            videoLog("请先播放视频");
            return;
        }
        player->setPosition(sec);

        // TODO(llhsdmd@gmail.com,BusyStudent): 增加弹幕是否装载的判定
        // vcanvas->setDanmakuPosition(sec);

        // TODO(BusyStudent): 字幕同弹幕一样
    }

    void setSkipStep(int sec) {
        skipStep = sec;
    }
    
    int position() {
        if (!player->hasVideo()) {
            videoLog("请先播放视频");
            return 0;
        }
        return player->position();
    }

    void stop() {
        if (player->isPlaying()) {
            qDebug()  << "stop : " << video->title() << "\n position : " << position();
            video->setStatus("position", position());
            player->stop();
        }
    }

private:
     /**
     * @brief 视频进度条
     */
    void connectVideoProgressBar() {
        videoProgressBar->setDisabled(true);
        // 处理来自进度条的请求
        QWidget::connect(videoProgressBar, &CustomSlider::sliderMoved, self, [this](int position){
            if (player->isSeekable()) {
                player->setPosition(position);
            } else {
                if (!player->hasVideo()) {
                    videoLog("请先播放视频");
                } else {
                    videoLog("无效操作");
                }
            }
        });

        // 同步更新进度条的进度信息
        QWidget::connect(player, &NekoMediaPlayer::durationChanged, self, [this](qreal sec){
            videoProgressBar->setRange(0, sec);
        });

        QWidget::connect(player, &NekoMediaPlayer::seekableChanged, self, [this](bool v) {
            videoProgressBar->setEnabled(v);
        });

        QWidget::connect(player, &NekoMediaPlayer::positionChanged, self, [this](int value){
            auto total = timeFormat(player->duration());
            auto current = timeFormat(value);
            if (total.length() > current.length()) {
                current = QString("%1:%2").
                    arg(0,total.length() - current.length() - 1, 10, QLatin1Char('0')).
                    arg(current);
            }
            ui_videoSetting->videoTimeLabel->setText(current + "/" + total);
            if (videoProgressBar->isSliderDown()) {
                return;
            }
            videoProgressBar->setValue(value);
        });

        QWidget::connect(videoProgressBar, &CustomSlider::tipBeforeShow, self, [this](QLabel *tipLabel, int value){
            tipLabel->setText(timeFormat(value));
        });

        QWidget::connect(player, &NekoMediaPlayer::bufferedDurationChanged, self, [this](double sec){
            videoProgressBar->setPreloadValue(sec + position());
        });
        // 结束信号
        QWidget::connect(player, &NekoMediaPlayer::errorOccurred, self, [this](NekoMediaPlayer::Error error, const QString &errorString){
            videoLog(errorString);
            emit self->finished();
        });
        QWidget::connect(player, &NekoMediaPlayer::mediaStatusChanged, self, [this](NekoMediaPlayer::MediaStatus status){
            switch (status)
            {
            case NekoMediaPlayer::MediaStatus::InvalidMedia:
                videoLog("非法文件");
                emit self->invalidVideo(video);
            case NekoMediaPlayer::MediaStatus::EndOfMedia:
                emit self->finished();
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
        QWidget::connect(volumeSetting, &VolumeSettingWidget::showed, self, [this](){
            videoSetting->show();
            videoSetting->setDefualtHideTime(std::numeric_limits<int>::max());
        });
        // 连接音量调节条隐藏：恢复视频进度条自动隐藏
        QWidget::connect(volumeSetting, &VolumeSettingWidget::hided, self, [this](){
            videoSetting->setDefualtHideTime(100);
            videoSetting->hideLater(5000);
        });
        // 按钮图标并更新程序播放音量
        QWidget::connect(audio, &NekoAudioOutput::volumeChanged, self, [this](float value){
            volumeSetting->setValue((value + 0.005) * 100);
        });
        // 连接音量调节条调节：同步音量
        QWidget::connect(volumeSetting, &VolumeSettingWidget::sliderMoved, self, [this](int value) {
            audio->setVolume(value / 100.0);
        });
        // 连接视频播放进度条隐藏：隐藏自己
        QWidget::connect(videoSetting, &PopupWidget::hided, volumeSetting, &VolumeSettingWidget::hide);
        // 连接音量按钮点击：切换静音-有声模式
        QWidget::connect(ui_videoSetting->voiceSettingButton, &QToolButton::clicked, self,  [this, volume = 0.0]() mutable {
            if (ui_videoSetting->voiceSettingButton->isChecked()) {
                volume = audio->volume();
                audio->setVolume(0);
            } else {
                audio->setVolume(volume);
            }
        });
        // 同步音量按钮图标
        QWidget::connect(volumeSetting, &VolumeSettingWidget::valueChanged, self, [this, flag = -1](int value) mutable {
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
                pause();
            } else {
                resume();
            }
        });
        QWidget::connect(player, &NekoMediaPlayer::playbackStateChanged, self, [this](NekoMediaPlayer::PlaybackState status){
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
    }

    /**
     * @brief 播放状态恢复
     * 
     */
    void connectVideoStatus() {
        QWidget::connect(player, &NekoMediaPlayer::mediaStatusChanged, self, [this](NekoMediaPlayer::MediaStatus status){
            switch (status)
            {
            case NekoMediaPlayer::MediaStatus::LoadedMedia:
                if (video->containsStatus("position")) {
                setPosition(video->getStatus<int>("position"));
                }
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
        QWidget::connect(videoSetting, &PopupWidget::hided, settings, &FullSettingWidget::hide);
        QWidget::connect(settings, &FullSettingWidget::showed, self, [this](){
            videoSetting->show();
            videoSetting->setDefualtHideTime(std::numeric_limits<int>::max());
        });
        QWidget::connect(settings, &FullSettingWidget::hided, self, [this](){
            videoSetting->setDefualtHideTime(100);
            videoSetting->hideLater(5000);
        });
        QWidget::connect(ui_videoSetting->settingButton, &QToolButton::clicked, settings, [this](){
            if (settings->isHidden()) {
                settings->show();
                settings->hideLater(5000);
            } else {
                settings->hide();
            }
        });
        settings->setupSetting(self);
    }

public:
    VideoCanvas* vcanvas;
    NekoMediaPlayer* player;
    NekoAudioOutput* audio;

    PopupWidget* videoSetting = nullptr;
    Ui::VideoSettingView* ui_videoSetting = nullptr;
    CustomSlider* videoProgressBar = nullptr;

    VolumeSettingWidget* volumeSetting = nullptr;

    FullSettingWidget* settings = nullptr;

    PopupWidget* logWidget = nullptr;
    Ui::CustomLabel *ui_logView = nullptr;
    VideoBLLPtr video;

    int skipStep = 10;

private:
    VideoWidget *self;
    QVBoxLayout* mainLayout;
};

VideoWidget::VideoWidget(QWidget* parent) : QWidget(parent), d(new VideoWidgetPrivate(this)) {
    d->setupUi();
    d->connect();
    d->setupShortcut();

    setMinimumSize(100,75);
    setAttribute(Qt::WA_Hover);                  // 启动鼠标悬浮追踪
    
    d->update();
}

void VideoWidget::resizeEvent(QResizeEvent* event) {
    d->vcanvas->resize(size());
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
    if (d->videoSetting != nullptr && obj == d->ui_videoSetting->voiceSettingButton) {
        if (event->type() == QEvent::Enter) {
            d->volumeSetting->show();
        } else if (event->type() == QEvent::Leave) {
            d->volumeSetting->hideLater(100);
        }
    }

    return QWidget::eventFilter(obj, event);
}

void VideoWidget::leaveEvent(QEvent* event) {
    d->videoSetting->hideLater();
    QWidget::leaveEvent(event);
}

void VideoWidget::mouseMoveEvent(QMouseEvent* event) {
    QMetaObject::invokeMethod(this, [this](){
            d->videoSetting->show();
            d->videoSetting->hideLater(5000);
        }, Qt::QueuedConnection);
}

void VideoWidget::playVideo(const VideoBLLPtr video) {
    d->play(video);
}


void VideoWidget::stop() {
    d->stop();
}

void VideoWidget::videoLog(const QString& info) {
    d->videoLog(info);
}

void VideoWidget::setPlaybackRate(qreal v) {
    // TODO(llhsdmd) : setPlaybackRate
    qInfo() << "TODO(setPlaybackRate)";
}
// 画面设置
void VideoWidget::setAspectRationMode(ScalingMode mode) {
    // TODO(llhsdmd) : setAspectRationMode
    qInfo() << "TODO(setAspectRationMode)";
}
void VideoWidget::RotationScreen(Rotation direction) {
    // TODO(llhsdmd) : RotationScreen
    qInfo() << "TODO(RotationScreen)";
}
void VideoWidget::setImageQualityEnhancement(bool v) {
    // TODO(llhsdmd) : setImageQualityEnhancement
    qInfo() << "TODO(setImageQualityEnhancement)";
}
// 色彩设置
void VideoWidget::setBrightness(int v) {
    // TODO(llhsdmd) : setBrightness
    qInfo() << "TODO(setBrightness)";
}
void VideoWidget::setContrast(int v) {
    // TODO(llhsdmd) : setContrast
    qInfo() << "TODO(setContrast)";
}
void VideoWidget::setHue(int v) {
    // TODO(llhsdmd) : setHue
    qInfo() << "TODO(setHue)";
}
void VideoWidget::setSaturation(int v) {
    // TODO(llhsdmd) : setSaturation
    qInfo() << "TODO(setSaturation)";
}
// 弹幕设置
void VideoWidget::setDanmakuShowArea(qreal OccupationRatio) {
    // TODO(llhsdmd) : setDanmakuShowArea
    qInfo() << "TODO(setDanmakuShowArea)";
}
void VideoWidget::setDanmakuSize(qreal ratio) {
    // TODO(llhsdmd) : setDanmakuSize
    qInfo() << "TODO(setDanmakuSize)";
}
void VideoWidget::setDanmakuSpeed(int speed) {
    // TODO(llhsdmd) : setDanmakuSpeed
    qInfo() << "TODO(setDanmakuSpeed)";
}
void VideoWidget::setDanmakuBackground(bool v) {
    // TODO(llhsdmd) : setDanmakuBackground
    qInfo() << "TODO(setDanmakuBackground)";
}
void VideoWidget::setDanmakuFont(const QFont& font) {
    // TODO(llhsdmd) : setDanmakuFont
    qInfo() << "TODO(setDanmakuFont)";
}
QFont VideoWidget::danmakuFont() {
    // TODO(llhsdmd) : danmakuFont
    qInfo() << "TODO(danmakuFont)";
    return QFont();
}
void VideoWidget::setDanmakuBackgroundTransparency(qreal percentage) {
    // TODO(llhsdmd) : setDanmakuBackgroundTransparency
    qInfo() << "TODO(setDanmakuBackgroundTransparency)";
}
void VideoWidget::setDanmakuBackgroundColor(QColor color) {
    // TODO(llhsdmd) : setDanmakuBackgroundColor
    qInfo() << "TODO(setDanmakuBackgroundColor)";
}
void VideoWidget::setDanmakuTransparency(qreal ratio) {
    // TODO(llhsdmd) : setDanmakuTransparency
    qInfo() << "TODO(setDanmakuTransparency)";
}
void VideoWidget::setDanmakuStroke(bool v) {
    // TODO(llhsdmd) : setDanmakuStroke
    qInfo() << "TODO(setDanmakuStroke)";
}
void VideoWidget::setDanmakuStrokeColor(QColor color) {
    // TODO(llhsdmd) : setDanmakuStrokeColor
    qInfo() << "TODO(setDanmakuStrokeColor)";
}
void VideoWidget::setDanmakuStrokeTransparency(qreal percentage) {
    // TODO(llhsdmd) : setDanmakuStrokeTransparency
    qInfo() << "TODO(setDanmakuStrokeTransparency)";
}
// 当前指针
VideoBLLPtr VideoWidget::currentVideo() {
    // TODO(llhsdmd) : currentVideo
    qInfo() << "TODO(currentVideo)";
    return nullptr;
}

// 字幕设置
void VideoWidget::setSubtitleSynchronizeTime(qreal t) {
    // TODO(llhsdmd) : setSubtitleSynchronizeTime
    qInfo() << "TODO(setSubtitleSynchronizeTime)";
}
void VideoWidget::setSubtitlePosition(qreal t) {
    // TODO(llhsdmd) : setSubtitlePosition
    qInfo() << "TODO(setSubtitlePosition)";
}
QFont VideoWidget::subtitleFont() {
    // TODO(llhsdmd) : subtitleFont
    qInfo() << "TODO(subtitleFont)";
    return QFont();
}
void VideoWidget::setSubtitleFont(const QFont& font) {
    // TODO(llhsdmd) : setSubtitleFont
    qInfo() << "TODO(setSubtitleFont)";
}
void VideoWidget::setSubtitleColor(const QColor& color) {
    // TODO(llhsdmd) : setSubtitleColor
    qInfo() << "TODO(setSubtitleColor)";
}

VideoWidget::~VideoWidget() {
    delete d;
}

int VideoWidget::skipStep() {
    return d->skipStep;
}

void VideoWidget::setSkipStep(int v) {
    d->setSkipStep(v);
}