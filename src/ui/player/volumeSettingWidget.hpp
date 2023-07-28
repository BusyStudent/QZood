#pragma once

#include "../util/widget/popupWidget.hpp"
#include "ui_volumeSettingView.h"

namespace Ui {
class VolumeSettingView;
}

class VolumeSettingWidget : public PopupWidget{
    Q_OBJECT
    public:
        VolumeSettingWidget(QWidget* parent = nullptr);

    Q_SIGNALS:
        void valueChanged(int value);
        void sliderMoved(int position);
    
    public Q_SLOTS:
        void setValue(int value);
    
    private:
        QScopedPointer<Ui::VolumeSettingView> ui;
};