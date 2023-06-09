#include "fullSettingWidget.hpp"
#include "ui_fullSettingView.h"

FullSettingWidget::FullSettingWidget(QWidget* parent) : PopupWidget(parent), ui(new Ui::FullSettingView) {
    ui->setupUi(this);
    _setupUi();
}

void FullSettingWidget::_setupUi() {
    // ====================播放列表====================
    ui->settingStackWidget->setCurrentIndex(0);
    ui->settingList->setCurrentRow(0);
    ui->settingList->setStyleSheet(R"(
        QListWidget#settingList{
            border: 1px solid #202020;
            background-color:  #202020;
            outline: 0px;
            padding-right: -1px;
            padding-left: -1px;
            padding-top: 8px;
        }
        QListWidget#settingList::item:hover{
            border: 1px solid #66b2ff;
            background-color: #66b2ff;
        }
        QListWidget#settingList::item {
            border: 1px solid #202020;
            color: white;	
            padding-top: 10px;
            padding-bottom: 10px;
        }
        QListWidget#settingList::item:selected{
            background-color: #66b2ff;
            show-decoration-selected: 0;
            margin: -1px  -2px;
            padding-left: 22px;
        })");

    // ====================播放设置====================
    // 播放速度去value / 10倍。
    ui->playbackRateBar->setRange(5, 30);
    connect(ui->playbackRateBar, &QSlider::valueChanged, this, [this](int value){
        ui->playbackRateBar->setToolTip(QString::number(_playbackRate(value), 'g', 2));
    });
    connect(ui->playbackRate05, &QPushButton::clicked, this, [this](bool checked) {
        ui->playbackRateBar->setValue(5);
    });
    connect(ui->playbackRate1, &QPushButton::clicked, this, [this](bool chlicked){
        ui->playbackRateBar->setValue(10);
    });
    connect(ui->playbackRate2, &QPushButton::clicked, this, [this](bool chlicked){
        ui->playbackRateBar->setValue(20);
    });
    connect(ui->playbackRate3, &QPushButton::clicked, this, [this](bool chlicked){
        ui->playbackRateBar->setValue(30);
    });

    // ====================画面设置====================
    // TODO(llhsdmd@gmail.com) : 画面设置ui功能实现
}

void FullSettingWidget::setSkipStep(int value) {
    // TODO(llhsdmd@gmail.com) : 设置视频跳过步长
}


void FullSettingWidget::_setup() {
    // ====================播放设置====================
    ui->playbackRateBar->setValue(10);
    ui->step10Button->setChecked(true);
}

void FullSettingWidget::show() {
    PopupWidget::show();
    ui->settingList->setFocus();
}

void FullSettingWidget::setPlaybackRate(double value) {
    ui->playbackRateBar->setValue(_playbackRate(value));
}