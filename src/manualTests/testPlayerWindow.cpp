#include "../ui/player/playerWidget.hpp"
#include "testregister.hpp"
#include "../ui/util/widget/customSlider.hpp"

#include <QTimer>

ZOOD_TEST_W(Ui, playerWindowTest) {
    PlayerWidget* playerWidget = new PlayerWidget();

    return playerWidget;
}

ZOOD_TEST(Ui, CustomSliderTest) {
    QWidget* ui = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    ui->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    CustomSlider* slider = new CustomSlider();
    slider->setStyleSheet(R"(
        QSlider::groove {
            border: none;
            height: 4px; 
            background: #A0A0A0A0; 
            margin: 1px -7px;
        }
        QSlider::handle	{
            background: transparent;
            border: none;
            width: 18px;
            margin: -5px 0;
            border-radius: 3px;
        }
        QSlider::handle:hover	{
            background: #66B2FF;
            border: none;
            width: 14px;
            margin: -5px 5px;
            border-radius: 7px;
        }

        QSlider::sub-page {
            border: none;
            height: 4px; 
            background: #66B2FF;
            margin: 1px 0px;
        }
    )");
    QWidget::connect(slider, &CustomSlider::valueChanged, slider, [slider](int value){
        QTimer::singleShot(1000, [slider, value](){
            slider->setPreloadValue(value + 10);
        });
    });

    layout->addStretch(1);
    layout->addWidget(slider);
    layout->addStretch(1);

    return ui;
}