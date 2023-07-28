#pragma once

#include "../util/widget/popupWidget.hpp"
#include "../../BLL/data/videoBLL.hpp"

class FullSettingWidgetPrivate;
class VideoWidget;

class FullSettingWidget : public PopupWidget {
    Q_OBJECT
    public:
        FullSettingWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::Popup);
        ~FullSettingWidget();
        void setupSetting(VideoWidget *VideoWidget);
        void initDanmakuSetting(VideoBLLPtr video);
        void initSubtitleSetting(VideoBLLPtr video);

    public Q_SLOTS:
        void show();

    private:
        QScopedPointer<FullSettingWidgetPrivate> d;
};