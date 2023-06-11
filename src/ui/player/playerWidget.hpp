#pragma once

#include <QWidget>
#include <QToolButton>
#include <QMenu>
#include <QTimer>

#include "../common/customizeTitleWidget.hpp"
#include "../common/popupWidget.hpp"
#include "volumeSettingWidget.hpp"
#include "videoWidget.hpp"
#include "fullSettingWidget.hpp"
#include "../common/customSlider.hpp"

namespace Ui {
class PlayerView;
class VideoSettingView;
}

class VideoSettingWidget : public PopupWidget {
    Q_OBJECT
    public:
        VideoSettingWidget(QWidget* parent = nullptr);

    public Q_SLOTS:
        void showLog(const QString& info);

    private:
        Ui::VideoSettingView* ui = nullptr;
        QTimer* timer = nullptr;
        CustomSlider* videoProgressBar = nullptr;

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
        void showEvent(QShowEvent* evnt) override;

    public Q_SLOTS:
        void clearPlayList();
        void playVideo(const QString& url);
        // void addVideoToPlayList();

    private:
        void _setupUi();
        void _setupProgressBar();
        void _setupVolumeSetting();
        void _setupVideoPlay();
        void _setupPlayList();
        bool _doLater(std::function<void()> func);

    private:
        Ui::PlayerView* ui = nullptr;
        VideoWidget* videoWidget = nullptr;
        VideoSettingWidget* videoSetting = nullptr;

        VolumeSettingWidget* volumeSetting = nullptr;
        FullSettingWidget* settings = nullptr;
};