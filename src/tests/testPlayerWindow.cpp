#include "../ui/player/playerWidget.hpp"
#include "testregister.hpp"

ZOOD_TEST_W(Ui, playerWindowTest) {
    PlayerWidget* playerWidget = new PlayerWidget();

    return playerWidget;
}

ZOOD_TEST(Ui, VolumeSettingWidgetTest) {
    VolumeSettingWidget* ui = new VolumeSettingWidget();
    ui->setHideAfterLeave(false);
    return ui;
}

ZOOD_TEST(Ui, VideoSettingWidgetTest) {
    VideoSettingWidget* ui = new VideoSettingWidget();
    ui->setHideAfterLeave(false);
    return ui;
}

ZOOD_TEST(Ui, FullSettingWidgetTest) {
    FullSettingWidget* ui = new FullSettingWidget();
    ui->setHideAfterLeave(false);
    return ui;
}