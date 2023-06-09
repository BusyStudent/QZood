#include "../ui/player/playerWidget.hpp"
#include "testregister.hpp"

ZOOD_TEST_W(Ui, playerWindowTest) {
    PlayerWidget* playerWidget = new PlayerWidget();

    return playerWidget;
}

ZOOD_TEST(Ui, VideoWidgetTest) {
    VideoWidget* ui = new VideoWidget();

    return ui;
}