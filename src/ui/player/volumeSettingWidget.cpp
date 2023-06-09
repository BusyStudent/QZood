#include "volumeSettingWidget.hpp"
#include "ui_volumeSettingView.h"

VolumeSettingWidget::VolumeSettingWidget(QWidget* parent) : PopupWidget(parent), ui(new Ui::VolumeSettingView) {
    ui->setupUi(this);
    connect(ui->volumeSettingBar, &QSlider::valueChanged, ui->volumeValue,[this](int value) {
        ui->volumeValue->setNum(value);
        emit valueChanged(value);
    });
    connect(ui->volumeSettingBar, &QSlider::sliderMoved, ui->volumeValue,[this](int value) {
        ui->volumeValue->setNum(value);
        emit sliderMoved(value);
    });
}

void VolumeSettingWidget::setValue(int value) {
    ui->volumeSettingBar->setValue(value);
}