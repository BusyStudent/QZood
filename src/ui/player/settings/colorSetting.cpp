#include "colorSetting.hpp"

#include "../videoWidget.hpp"
#include "../../../common/myGlobalLog.hpp"
#include "ui_colorSetting.h"

class ColorSettingPrivate {
public:
    ColorSettingPrivate(ColorSetting *self) : self(self), ui(new Ui::ColorSetting()) {
        ui->setupUi(self);
    }
    void setup(VideoWidget *videoWidget) {
        this->videoWidget = videoWidget;
        connectToVideoWidget();
    }
    void reset() {
        ui->brightnessBar->setValue(0);
        ui->contrastBar->setValue(0);
        ui->hueBar->setValue(0);
        ui->saturationBar->setValue(0);
    }

    void refresh() {
        if (nullptr != videoWidget) {
            auto video = videoWidget->currentVideo();
            if (nullptr != video) {
                int brightness = video->getStatus<int>("brightness").value_or(0);
                ui->brightnessBar->setValue(brightness);

                int contrast = video->getStatus<int>("contrast").value_or(0);
                ui->contrastBar->setValue(contrast);

                int hue = video->getStatus<int>("hue").value_or(0);
                ui->hueBar->setValue(hue);

                int saturation = video->getStatus<int>("saturation").value_or(0);
                ui->saturationBar->setValue(saturation);
            }
        }
     }

public:
    VideoWidget *videoWidget = nullptr;

private:
    void connectToVideoWidget() {
        // 色彩设置
        QWidget::connect(ui->brightnessBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            // TODO(llhsdmd) : setBrightness
            qInfo() << "TODO(setBrightness)";
            ui->brightnessLabel->setNum(value);
        });
        QWidget::connect(ui->contrastBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            // TODO(llhsdmd) : setContrast
            qInfo() << "TODO(setContrast)";
            ui->contrastLabel->setNum(value);
        });
        QWidget::connect(ui->hueBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            // TODO(llhsdmd) : setHue
            qInfo() << "TODO(setHue)";
            ui->hueLabel->setNum(value);
        });
        QWidget::connect(ui->saturationBar, &QSlider::valueChanged, videoWidget, [this](int value) {
            // TODO(llhsdmd) : setSaturation
            qInfo() << "TODO(setSaturation)";
            ui->saturationLabel->setNum(value);
        });
    }

private:
    ColorSetting *self;
    QScopedPointer<Ui::ColorSetting> ui;
};

ColorSetting::ColorSetting(QWidget *parent) : QWidget(parent), SettingItem(), d(new ColorSettingPrivate(this)) { 

}
ColorSetting::~ColorSetting() {

}
void ColorSetting::initialize(VideoWidget *videoWidget) {
    d->setup(videoWidget);
}
QString ColorSetting::title() {
    return "色彩";
}
QWidget *ColorSetting::widget() {
    return this;
}
void ColorSetting::refresh() {
    d->refresh();
}
void ColorSetting::reset() {
    d->reset();
}