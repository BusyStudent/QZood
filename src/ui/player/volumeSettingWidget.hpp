#pragma once

#include "../common/popupWidget.hpp"

namespace Ui {
class VolumeSettingView;
}

class VolumeSettingWidget : public PopupWidget{
    Q_OBJECT
    public:
        VolumeSettingWidget(QWidget* parent = nullptr);

    Q_SIGNALS:
        void valueChanged(int value);
    
    public Q_SLOTS:
        void setValue(int value);
    
    private:
        Ui::VolumeSettingView* ui;
};