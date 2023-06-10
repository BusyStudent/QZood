#include "playerWidget.hpp"
#include "ui_playerView.h"
#include "ui_videoSettingView.h"

#include <QMenuBar>
#include <QMouseEvent>
#include <climits>
#include <QMimeData>
#include <QTime>

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

VideoSettingWidget::VideoSettingWidget(QWidget* parent) : PopupWidget(parent), ui(new Ui::VideoSettingView) {
    ui->setupUi(this);

    videoProgressBar = new CustomSlider();
    videoProgressBar->setObjectName("videoProgressBar");
    static_cast<QVBoxLayout*>(layout())->insertWidget(1, videoProgressBar);

    timer = new QTimer(this);
    timer->setSingleShot(true);

    connect(timer, &QTimer::timeout, this, [this](){
        if (ui->infoBoard->count()) {
            auto item = ui->infoBoard->takeItem(0);
            delete item;
            ui->infoBoard->update();
            QMetaObject::invokeMethod(this, [this](){
                timer->start(1000);
            }, Qt::QueuedConnection);
        }
    });

    connect(videoProgressBar, &CustomSlider::tipBeforeShow, this, [this](QLabel *label, int value){
        label->setText(timeFormat(value));
        label->updateGeometry();
    }, Qt::ConnectionType::DirectConnection);
}

void VideoSettingWidget::showLog(const QString& info) {
    timer->start(1000);
    ui->infoBoard->addItem(info);
}

void PlayerWidget::playVideo(const QString& url) {
    // TODO(llhsdmd@gmail.com) : 设置播放的默认参数

    videoWidget->playVideo(url);
}

void PlayerWidget::showEvent(QShowEvent* event) {
    this->setAttribute(Qt::WA_Mapped);
}

void PlayerWidget::clearPlayList() {
    ui->playlist->clear();
}

PlayerWidget::PlayerWidget(QWidget* parent) : CustomizeTitleWidget(parent), ui(new Ui::PlayerView()) {
    ui->setupUi(this);
    _setupUi();
    setWindowTitle("QZoodPlayer");

    createShadow(ui->containerWidget);

    connect(ui->minimizeButton, &QToolButton::clicked, this, [this](){
        showMinimized();
    });
    connect(ui->maximizeButton, &QToolButton::clicked, this, [this](){
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(ui->closeButton, &QToolButton::clicked, this, [this](){
        close();
    });
}

void PlayerWidget::dragEnterEvent(QDragEnterEvent *event) {

    CustomizeTitleWidget::dragEnterEvent(event);
}

void PlayerWidget::dropEvent(QDropEvent *event) {

    CustomizeTitleWidget::dropEvent(event);
}

void PlayerWidget::resizeEvent(QResizeEvent *event) {
    static bool is_maximized = false;
	if (isMaximized() && !is_maximized) {
        is_maximized = true;
		ui->maximizeButton->setIcon(QIcon(":/icons/minimize_white.png"));
	} else if (is_maximized) {
        is_maximized = false;
		ui->maximizeButton->setIcon(QIcon(":/icons/maximize_white.png"));
	}

    CustomizeTitleWidget::resizeEvent(event);
}

bool PlayerWidget::eventFilter(QObject* obj,QEvent* event) {
    if (obj == ui->videoPlayContainer){
        if(event->type() == QEvent::Type::Resize) {
            videoWidget->resize(ui->videoPlayContainer->size());
        }
    } else if (obj == videoSetting->ui->voiceSettingButton) {
        if (event->type() == QEvent::Enter) {
            volumeSetting->show();
        } else if (event->type() == QEvent::Leave) {
            volumeSetting->hideLater(100);
        }
    }

    return QWidget::eventFilter(obj, event);
}

void PlayerWidget::leaveEvent(QEvent* event) {
    videoSetting->hideLater();

    CustomizeTitleWidget::leaveEvent(event);
}

void PlayerWidget::mouseMoveEvent(QMouseEvent* event) {
    auto topLeft = ui->videoPlayContainer->mapToGlobal(QPoint(0, 0));
    auto bottomRight = ui->videoPlayContainer->mapToGlobal(QPoint(ui->videoPlayContainer->size().width(), ui->videoPlayContainer->size().height()));
    if (QRect(topLeft, bottomRight).contains(event->globalPos())) {
        QMetaObject::invokeMethod(this, [this](){
            videoSetting->show();
            videoSetting->hideLater(5000);
        });
    }

    if (movingStatus() && ui->titleBar->geometry().contains(event->pos())) {
        window()->move(event->globalPos() - diff_pos);
        event->accept();
    }

    // 刷新窗体状态
    CustomizeTitleWidget::mouseMoveEvent(event);
}

void PlayerWidget::_setupProgressBar() {
    videoSetting->videoProgressBar->setDisabled(true);


    connect(videoWidget, &VideoWidget::durationChanged, this, [this](int sec){
        videoSetting->videoProgressBar->setRange(0, sec);
    });

    connect(videoWidget, &VideoWidget::stoped, this, [this](){
        videoSetting->videoProgressBar->setDisabled(true);
    });

    connect(videoWidget, &VideoWidget::seekableChanged, this, [this](bool v) {
        videoSetting->videoProgressBar->setEnabled(v);
    });

    connect(videoWidget, &VideoWidget::positionChanged, this, [this](int value){
        auto total = timeFormat(videoWidget->duration());
        auto current = timeFormat(value);
        if (total.length() > current.length()) {
            current = QString("%1:%2").
                arg(0,total.length() - current.length() - 1, 10, QLatin1Char('0')).
                arg(current);
        }
        videoSetting->ui->videoTimeLabel->setText(current + "/" + total);

        if (videoSetting->videoProgressBar->isSliderDown()) {
            return;
        }
        videoSetting->videoProgressBar->setValue(value);
    });

    connect(videoSetting->videoProgressBar, &QSlider::sliderMoved, videoWidget, [this](int position){
        videoWidget->setPosition(position);
    });
}

void PlayerWidget::_setupVolumeSetting() {
    // 注册音量控制按钮到本类用于处理音量调节条的弹出
    videoSetting->ui->voiceSettingButton->installEventFilter(this);
    volumeSetting = new VolumeSettingWidget();
    // 连接音量调节条显示：定义弹出窗口显示时进度条不自动隐藏。
    connect(volumeSetting, &VolumeSettingWidget::showed, this, [this](){
        videoSetting->show();
        videoSetting->setDefualtHideTime(std::numeric_limits<int>::max());
    });
    // 连接音量调节条隐藏：恢复视频进度条自动隐藏
    connect(volumeSetting, &VolumeSettingWidget::hided, this, [this](){
        videoSetting->setDefualtHideTime(100);
        videoSetting->hideLater(5000);
    });
    // 按钮图标并更新程序播放音量
    connect(videoWidget, &VideoWidget::volumeChanged, this, [this](int value){
        volumeSetting->setValue(value);
    });
    // 连接音量调节条调节：同步音量
    connect(volumeSetting, &VolumeSettingWidget::sliderMoved, this, [this](int value) {
        videoWidget->setVolume(value);
    });
    // 连接视频播放进度条隐藏：隐藏自己
    connect(videoSetting, &VideoSettingWidget::hided, volumeSetting, &VolumeSettingWidget::hide);
    // 连接音量按钮点击：切换静音-有声模式
    connect(videoSetting->ui->voiceSettingButton, &QToolButton::clicked, this,  [this, volume = 0]() mutable {
        if (videoSetting->ui->voiceSettingButton->isChecked()) {
            volume = videoWidget->volume();
            videoWidget->setVolume(0);
        } else {
            videoWidget->setVolume(volume);
        }
    });
    // 同步音量按钮图标
    connect(volumeSetting, &VolumeSettingWidget::valueChanged, this, [this, flag = -1](int value) mutable {
        if (value == 0 && flag != 0) {
            flag = 0;
            QIcon icon;
            icon.addFile(QString::fromUtf8(":/icons/mute_white.png"), QSize(), QIcon::Normal, QIcon::Off);
            videoSetting->ui->voiceSettingButton->setIcon(icon);
        } else if (value > 0 && value <= 33 && flag != 1) {
            flag = 1;
            QIcon icon;
            icon.addFile(QString::fromUtf8(":/icons/volume_1_white.png"), QSize(), QIcon::Normal, QIcon::Off);
            videoSetting->ui->voiceSettingButton->setIcon(icon);
        } else if (value > 33 && value <= 66 && flag != 2) {
            flag = 2;
            QIcon icon;
            icon.addFile(QString::fromUtf8(":/icons/volume_2_white.png"), QSize(), QIcon::Normal, QIcon::Off);
            videoSetting->ui->voiceSettingButton->setIcon(icon);
        } else if (value > 66 && flag != 3){
            flag = 3;
            QIcon icon;
            icon.addFile(QString::fromUtf8(":/icons/volume_3_white.png"), QSize(), QIcon::Normal, QIcon::Off);
            videoSetting->ui->voiceSettingButton->setIcon(icon);
        }
    });

    // 设置控件属性
    volumeSetting->setParent(ui->videoPlayContainer);
    volumeSetting->setAssociateWidget(videoSetting->ui->voiceSettingButton, PopupWidget::Direction::TOP);
    volumeSetting->setValue(videoWidget->volume());
}

void PlayerWidget::_setupPlayList() {
    // TODO(llhsdmd@gmail.com) : 
}


void PlayerWidget::_setupVideoPlay() {
    videoWidget->lower();
    videoWidget->setWindowFlag(Qt::WindowTransparentForInput, true);
    videoWidget->setAttribute(Qt::WA_PaintOnScreen);
    // 连接播放按钮的行为
    videoSetting->ui->playerButton->setCheckable(false);
    videoSetting->ui->playerButton->setChecked(false);
    connect(videoSetting->ui->playerButton, &QToolButton::clicked, this, [this](bool checked){
        if (!checked) {
            videoWidget->pauseVideo();
        } else {
            videoWidget->resumeVide();
        }
    });
    // 连接异常信息显示
    connect(videoWidget, &VideoWidget::runError, videoSetting, &VideoSettingWidget::showLog);
    connect(videoWidget, &VideoWidget::playing, this, [this](){
        videoSetting->ui->playerButton->setCheckable(true);
        videoSetting->ui->playerButton->setChecked(true);
    });
    connect(videoWidget, &VideoWidget::paused, this, [this](){
        videoSetting->ui->playerButton->setCheckable(true);
        videoSetting->ui->playerButton->setChecked(false);
    });
    connect(videoWidget, &VideoWidget::stoped, this, [this](){
        videoSetting->ui->playerButton->setCheckable(false);
        videoSetting->ui->playerButton->setChecked(false);
    });

    // TODO(llhsdmd@gmail.com) : 切换集数，
}


void PlayerWidget::_setupUi() {
    // 设置默认划分比例
    ui->splitter->setStretchFactor(0, 10);
    ui->splitter->setStretchFactor(1, 1);

    // 设置窗口置顶功能
    connect(ui->onTopButton, &QToolButton::clicked, this, [this, flags = windowFlags()]() mutable {
        if (ui->onTopButton->isChecked()) {
            flags = windowFlags();
            setWindowFlags(flags | Qt::WindowStaysOnTopHint);
            show();
        } else {
            setWindowFlags(flags);
            show();
        }
    });
    
    // 实例化视频播放核心控件
    videoWidget = new VideoWidget(ui->videoPlayContainer);
    videoWidget->resize(ui->videoPlayContainer->size());
    ui->videoPlayContainer->installEventFilter(this);

    // 实例化视频播放进度条及设置栏
    videoSetting = new VideoSettingWidget();
    ui->videoPlayContainer->layout()->addWidget(videoSetting);
    videoSetting->show();


    // 设置可以自动隐藏的进度条栏
    _setupProgressBar();

    // 设置音量调节控件
    _setupVolumeSetting();
    
    // 链接视频信号
    _setupVideoPlay();

    // 设置控件
    videoSetting->ui->settingButton->installEventFilter(this);
    settings = new FullSettingWidget(ui->videoPlayContainer);
    settings->setAssociateWidget(videoSetting->ui->settingButton, PopupWidget::TOP);
    settings->setHideAfterLeave(false);
    connect(videoSetting, &VideoSettingWidget::hided, settings, &FullSettingWidget::hide);
    connect(settings, &FullSettingWidget::showed, this, [this](){
        videoSetting->show();
        videoSetting->setDefualtHideTime(std::numeric_limits<int>::max());
    });
    connect(settings, &FullSettingWidget::hided, this, [this](){
        videoSetting->setDefualtHideTime(100);
        videoSetting->hideLater(5000);
    });
    connect(videoSetting->ui->settingButton, &QToolButton::clicked, settings, [this](){
        if (settings->isHidden()) {
            settings->show();
            settings->hideLater(5000);
        } else {
            settings->hide();
        }
    });
}

bool PlayerWidget::_doLater(std::function<void()> func) {
    return QMetaObject::invokeMethod(this, func, Qt::QueuedConnection);
}

PlayerWidget::~PlayerWidget() {
}