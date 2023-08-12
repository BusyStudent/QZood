#include "playSetting.hpp"

#include "../videoWidget.hpp"
#include "../../../common/myGlobalLog.hpp"
#include "ui_playSetting.h"

class PlaySettingPrivate {
public:
    PlaySettingPrivate(PlaySetting *self) : self(self), ui(new Ui::PlaySetting()) {
        ui->setupUi(self);
        // ====================播放设置====================
        // 播放速度去value / 10倍。
        ui->playbackRateBar->setRange(5, 30);
    }

    void setup(VideoWidget *videoWidget) {
        this->videoWidget = videoWidget;
        connectToVideoWidget();
    }

    void reset() {
        ui->step10Button->click();
        ui->playbackRate1->click();
    }

    void refresh() {
        if (nullptr != videoWidget) {
            auto video = videoWidget->currentVideo();
            if (nullptr != video) {
                int skipStep = video->getStatus<int>("skipStep").value_or(10);
                switch(skipStep) {
                    case 5: ui->step5Button->setChecked(true); break;
                    case 10: ui->step10Button->setChecked(true); break;
                    case 30: ui->step30Button->setChecked(true); break;
                    default:
                        ui->step10Button->setChecked(false);
                        ui->step30Button->setChecked(false);
                        ui->step5Button->setChecked(false);
                        videoWidget->setSkipStep(skipStep);
                        break;
                }
            }
        }
    }

public:
    VideoWidget *videoWidget = nullptr;

private:
    void connectToVideoWidget() {
        // 播放条显示的值
        QWidget::connect(ui->playbackRateBar, &QSlider::valueChanged, self, [this](int value){
            ui->playbackRateBar->setToolTip(QString::number(value / 10.0, 'g', 2));
            videoWidget->setPlaybackRate(value / 10.0);
        });
        // 连接播放器到
        QWidget::connect(ui->playbackRate05, &QPushButton::clicked, self, [this](bool checked) {
            ui->playbackRateBar->setValue(5);
        });
        QWidget::connect(ui->playbackRate1, &QPushButton::clicked, self, [this](bool chlicked){
            ui->playbackRateBar->setValue(10);
        });
        QWidget::connect(ui->playbackRate2, &QPushButton::clicked, self, [this](bool chlicked){
            ui->playbackRateBar->setValue(20);
        });
        QWidget::connect(ui->playbackRate3, &QPushButton::clicked, self, [this](bool chlicked){
            ui->playbackRateBar->setValue(30);
        });
        QWidget::connect(ui->step5Button, &QRadioButton::clicked, videoWidget, [this](bool clicked){
            videoWidget->setSkipStep(5);
        });
        QWidget::connect(ui->step10Button, &QRadioButton::clicked, videoWidget, [this](bool clicked){
            videoWidget->setSkipStep(10);
        });
        QWidget::connect(ui->step30Button, &QRadioButton::clicked, videoWidget, [this](bool clicked){
            videoWidget->setSkipStep(30);
        });
    }

private:
    PlaySetting *self;
    QScopedPointer<Ui::PlaySetting> ui;
};

PlaySetting::PlaySetting(QWidget *parent) : QWidget(parent), SettingItem(), d(new PlaySettingPrivate(this)) {
}
PlaySetting::~PlaySetting() {

}
void PlaySetting::initialize(VideoWidget *videoWidget) {
    d->setup(videoWidget);
}
QString PlaySetting::title() {
    return "播放";
}
QWidget *PlaySetting::widget() {
    return this;
}
void PlaySetting::refresh() {
    d->refresh();
}
void PlaySetting::reset() {
    d->reset();
}