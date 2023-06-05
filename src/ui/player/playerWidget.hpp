#pragma once

#include <QWidget>
#include <QToolButton>
#include <QMenu>

#include "../common/customizeTitleWidget.hpp"
#include "../common/popupWidget.hpp"
#include "volumeSettingWidget.hpp"
#include "videoWidget.hpp"
#include "fullSettingWidget.hpp"

namespace Ui {
class PlayerView;
class VideoSettingView;
}

class VideoSettingWidget : public PopupWidget {
    Q_OBJECT
    public:
        VideoSettingWidget(QWidget* parent = nullptr);
    
    private:
        Ui::VideoSettingView* ui;

    friend class PlayerWidget;
};

class PlayerWidget : public CustomizeTitleWidget {
    Q_OBJECT
    public:
        PlayerWidget(QWidget* parent = nullptr);
        virtual ~PlayerWidget();

    public:
        void resizeEvent(QResizeEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        bool eventFilter(QObject* obj,QEvent* event) override;
        void leaveEvent(QEvent* event) override;

    public Q_SLOTS:
        // void clearPlayList();
        // void addVideoToPlayList();

    private:
        void _setupUi();
        bool _doLater(std::function<void()> func);
    private:
        Ui::PlayerView* ui;
        VideoWidget* videoWidget;
        VideoSettingWidget* videoSetting;

        VolumeSettingWidget* volumeSetting;
        FullSettingWidget* settings;
};