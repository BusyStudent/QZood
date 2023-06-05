#include "playerWidget.hpp"
#include "ui_playerView.h"
#include "ui_videoSettingView.h"

#include <QMenuBar>
#include <QMouseEvent>
#include <climits>

VideoSettingWidget::VideoSettingWidget(QWidget* parent) : PopupWidget(parent), ui(new Ui::VideoSettingView) {
    ui->setupUi(this);
}

PlayerWidget::PlayerWidget(QWidget* parent) : CustomizeTitleWidget(parent), ui(new Ui::PlayerView()) {
    ui->setupUi(this);
    _setupUi();

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

void PlayerWidget::resizeEvent(QResizeEvent *event) {
    static bool is_maximized = false;
	if (isMaximized() && !is_maximized) {
        is_maximized = true;
		ui->maximizeButton->setIcon(QIcon(":/icons/minimize_white.png"));
	} else if (is_maximized) {
        is_maximized = false;
		ui->maximizeButton->setIcon(QIcon(":/icons/maximize_white.png"));
	}
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
    if (ui->videoPlayContainer->underMouse()) {
        QMetaObject::invokeMethod(this, [this](){
            videoSetting->show();
            videoSetting->hideLater(5000);
        });
    } else {
        CustomizeTitleWidget::mouseMoveEvent(event);
    }
}

void PlayerWidget::_setupUi() {
    // 设置默认划分比例
    ui->splitter->setStretchFactor(0, 10);
    ui->splitter->setStretchFactor(1, 1);
    
    // 设置视频播放核心控件
    videoWidget = new VideoWidget(ui->videoPlayContainer);
    videoWidget->resize(ui->videoPlayContainer->size());
    ui->videoPlayContainer->installEventFilter(this);

    // 设置可以自动隐藏的进度条栏
    videoSetting = new VideoSettingWidget();
    ui->videoPlayContainer->layout()->addWidget(videoSetting);
    videoSetting->show();

    // 设置音量调节控件
    videoSetting->ui->voiceSettingButton->installEventFilter(this);
    volumeSetting = new VolumeSettingWidget();
    volumeSetting->setParent(ui->videoPlayContainer);
    volumeSetting->setValue(videoWidget->volume());
    volumeSetting->setAssociateWidget(videoSetting->ui->voiceSettingButton, PopupWidget::Direction::TOP);
    connect(videoSetting, &VideoSettingWidget::hided, volumeSetting, &VolumeSettingWidget::hide);
    connect(volumeSetting, &VolumeSettingWidget::showed, this, [this](){
        videoSetting->show();
        videoSetting->setDefualtHideTime(std::numeric_limits<int>::max());
    });
    connect(volumeSetting, &VolumeSettingWidget::hided, this, [this](){
        videoSetting->setDefualtHideTime(100);
        videoSetting->hideLater(5000);
    });
    connect(volumeSetting, &VolumeSettingWidget::valueChanged, this, [this, flag = -1](int value) mutable {
        if (value == 0 && flag != 0) {
            flag = 0;
            QIcon icon;
            icon.addFile(QString::fromUtf8(":/icons/volume_0_white.png"), QSize(), QIcon::Normal, QIcon::Off);
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
        videoWidget->setVolume(value);
    });

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
        ui->videoPlayContainer->setFocus();
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