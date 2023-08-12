#pragma once

#include "../util/widget/popupWidget.hpp"
#include "../../BLL/data/videoBLL.hpp"

#include <QPushButton>

class FullSettingWidgetPrivate;
class VideoWidget;
class SettingItem;

class FullSettingWidget : public PopupWidget {
    Q_OBJECT
    public:
        FullSettingWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::Popup);
        ~FullSettingWidget();
        void setupSetting(VideoWidget *VideoWidget);
        void addSettingItem(SettingItem *item, VideoWidget *VideoWidget);
        void refresh();

        void getColor(QPushButton* colorButton, std::function<void(const QColor& color)> func);
        void setColorForButton(QPushButton* colorButton,const QColor &color);

    public Q_SLOTS:
        void show();

    private:
        QScopedPointer<FullSettingWidgetPrivate> d;
};