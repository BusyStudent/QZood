#pragma once

#include "../common/popupWidget.hpp"

class FullSettingWidgetPrivate;
class VideoWidget;

class FullSettingWidget : public PopupWidget {
    Q_OBJECT
    public:
        FullSettingWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::Popup);
        void setupSetting(VideoWidget *VideoWidget);

    public Q_SLOTS:
        void show();

    private:
        FullSettingWidgetPrivate* d;
};