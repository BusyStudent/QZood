#pragma once

#include <QWidget>
#include <QToolButton>
#include <QMenu>
#include <QTimer>
#include <QListWidgetItem>

#include "../common/customizeTitleWidget.hpp"
#include "../common/popupWidget.hpp"
#include "volumeSettingWidget.hpp"
#include "videoWidget.hpp"
#include "fullSettingWidget.hpp"
#include "../common/customSlider.hpp"
#include "../../net/datalayer.hpp"

namespace Ui {
class PlayerView;
class VideoSettingView;
}

struct Video;

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

class PlayerWidget final : public CustomizeTitleWidget {
    Q_OBJECT
    public:
        PlayerWidget(QWidget* parent = nullptr);
        virtual ~PlayerWidget();

    public:
        void resizeEvent(QResizeEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        bool eventFilter(QObject* obj,QEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;

    private:
        void _setupUi();
        void _setupProgressBar();
        void _setupVolumeSetting();
        void _setupVideoPlay();
        void _setupPlayList();
        void _playVideo(Video* video);
        void _addVideoToList(Video* video);
        QListWidgetItem* _nextVideo();

        bool _doLater(std::function<void()> func);

    private:
        Ui::PlayerView* ui = nullptr;
        VideoWidget* videoWidget = nullptr;
        VideoSettingWidget* videoSetting = nullptr;

        VolumeSettingWidget* volumeSetting = nullptr;
        FullSettingWidget* settings = nullptr;

        QMap<QListWidgetItem*, Video*> videos;
};