#include "screenSetting.hpp"

#include "../videoWidget.hpp"
#include "../../../common/myGlobalLog.hpp"
#include "ui_screenSetting.h"

class ScreenSettingPrivate {
public:
    ScreenSettingPrivate(ScreenSetting *self) : self(self), ui(new Ui::ScreenSetting()) {
        ui->setupUi(self);
    }
    void setup(VideoWidget *videoWidget) {
        this->videoWidget = videoWidget;
        connectToVideoWidget();
    }
    void reset() {
        ui->defaultAspectRatioButton->click();
        ui->imageQualityEnhancementButton->setCheckState(Qt::CheckState::Unchecked);
        ui->imageQualityEnhancementButton->stateChanged(Qt::CheckState::Unchecked);
    }

    void refresh() {
        // 刷新播放比例
        if (nullptr != videoWidget) {
            auto video = videoWidget->currentVideo();
            if (nullptr != video) {
                auto aspectRatio = video->getStatus<QString>("aspectRatio");
                if (aspectRatio.has_value()) {
                    if (aspectRatio.value() == "16:9") {
                        ui->aspectRation169Button->click();
                    } else if (aspectRatio.value() == "4:3") {
                        ui->aspectRation43Button->click();
                    } else if (aspectRatio.value() == "full") {
                        ui->aspectRationFullButton->click();
                    } else if (aspectRatio.value() == "default") {
                        ui->defaultAspectRatioButton->click();
                    }
                } else {
                    ui->defaultAspectRatioButton->click();
                }
            }

            auto imageQualityEnhancement = video->getStatus<bool>("imageQualityEnhancement");
            Qt::CheckState value = Qt::Unchecked;
            if (imageQualityEnhancement.has_value()) {
                value = imageQualityEnhancement.value() ? Qt::Checked : Qt::Unchecked;
            }
            ui->imageQualityEnhancementButton->setCheckState(value);
            ui->imageQualityEnhancementButton->stateChanged(value);
        }
    }

public:
    VideoWidget *videoWidget;

private:
    void connectToVideoWidget() {
        // 播放比例
        QWidget::connect(ui->defaultAspectRatioButton, &QRadioButton::clicked, videoWidget, [this](bool clicked){
            videoWidget->videoCanvas()->setAspectMode(VideoCanvas::KeepAspect);
        });
        QWidget::connect(ui->aspectRation43Button, &QRadioButton::clicked, videoWidget, [this](bool clicked){
            videoWidget->videoCanvas()->setAspectMode(VideoCanvas::_4x3);
        });
        QWidget::connect(ui->aspectRation169Button, &QRadioButton::clicked, videoWidget, [this](bool clicked){
            videoWidget->videoCanvas()->setAspectMode(VideoCanvas::_16x9);
        });
        QWidget::connect(ui->aspectRationFullButton, &QRadioButton::clicked, videoWidget, [this](bool clicked){
            videoWidget->videoCanvas()->setAspectMode(VideoCanvas::Filling);
        });
        // 画面旋转
        QWidget::connect(ui->clockwiseRotationButton, &QPushButton::clicked, videoWidget, [this](bool clicked) {
            // TODO(llhsdmd) : RotationScreen
            qInfo() << "TODO(RotationScreen)";
        });
        QWidget::connect(ui->anticlockwiseRotationButton, &QPushButton::clicked, videoWidget, [this](bool clicked) {
            // TODO(llhsdmd) : RotationScreen
            qInfo() << "TODO(RotationScreen)";
        });
        QWidget::connect(ui->horizontalFilpButton, &QPushButton::clicked, videoWidget, [this](bool clicked) {
            // TODO(llhsdmd) : RotationScreen
            qInfo() << "TODO(RotationScreen)";
        });
        QWidget::connect(ui->verticallyFilpButton, &QPushButton::clicked, videoWidget, [this](bool clicked) {
            // TODO(llhsdmd) : RotationScreen
            qInfo() << "TODO(RotationScreen)";
        });
        // 画质增强
        QWidget::connect(ui->imageQualityEnhancementButton, &QCheckBox::stateChanged, videoWidget, [this](int status) {
            // TODO(llhsdmd) : setImageQualityEnhancement
            qInfo() << "TODO(setImageQualityEnhancement)";
        });
    }

private:
    ScreenSetting *self;
    QScopedPointer<Ui::ScreenSetting> ui;
};

ScreenSetting::ScreenSetting(QWidget *parent) : QWidget(parent), SettingItem(), d(new ScreenSettingPrivate(this)) {

}
ScreenSetting::~ScreenSetting() {

}
void ScreenSetting::initialize(VideoWidget *videoWidget) {
    d->setup(videoWidget);
}
QString ScreenSetting::title() {
    return "画面";
}
QWidget *ScreenSetting::widget() {
    return this;
}
void ScreenSetting::refresh() {

}
void ScreenSetting::reset() {
    d->reset();
}