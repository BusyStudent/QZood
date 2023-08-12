#include "fullSettingWidget.hpp"
#include "ui_fullSettingView.h"

#include "videoWidget.hpp"

#include <QFileDialog>
#include <QColorDialog>

#include "../../common/myGlobalLog.hpp"
#include "../util/interface/settingItem.hpp"

class FullSettingWidgetPrivate {
public:
    FullSettingWidgetPrivate(FullSettingWidget *parent) : ui(new Ui::FullSettingView()), self(parent) {}
    ~FullSettingWidgetPrivate() { }

    void setupUi() {
        ui->setupUi(self);
        // ====================播放列表====================
        ui->settingStackWidget->setCurrentIndex(0);
        ui->settingList->setCurrentRow(0);
        ui->settingList->setStyleSheet(R"(
            QListWidget#settingList{
                border: 1px solid #202020;
                background-color:  #202020;
                outline: 0px;
                padding-right: -1px;
                padding-left: -1px;
                padding-top: 8px;
            }
            QListWidget#settingList::item:hover{
                border: 1px solid #66b2ff;
                background-color: #66b2ff;
            }
            QListWidget#settingList::item {
                border: 1px solid #202020;
                color: white;	
                padding-top: 10px;
                padding-bottom: 10px;
            }
            QListWidget#settingList::item:selected{
                background-color: #66b2ff;
                show-decoration-selected: 0;
                margin: -1px  -2px;
                padding-left: 22px;
            })");
    }

    void connect(VideoWidget* videoWidget) { }

    void update() { 
        for (auto item : mItems) {
            item->refresh();
        }
    }

    void addSettingItem(SettingItem *item, VideoWidget *videoWidget) {
            item->initialize(videoWidget);
            item->refresh();
            ui->settingList->addItem(item->title());
            ui->settingStackWidget->addWidget(item->widget());
            mItems.push_back(item);
    }

public:
    QScopedPointer<Ui::FullSettingView> ui;
    QList<SettingItem*> mItems;

private:
    FullSettingWidget *self;
};

FullSettingWidget::FullSettingWidget(QWidget* parent, Qt::WindowFlags f) : PopupWidget(parent, f), d(new FullSettingWidgetPrivate(this)) {
    d->setupUi();
}

FullSettingWidget::~FullSettingWidget() { }

void FullSettingWidget::addSettingItem(SettingItem *item, VideoWidget *videoWidget) {
    d->addSettingItem(item, videoWidget);
}

void FullSettingWidget::setupSetting(VideoWidget *videoWidget) {
    d->connect(videoWidget);
}

void FullSettingWidget::show() {
    PopupWidget::show();
}

void FullSettingWidget::getColor(QPushButton* colorButton, std::function<void(const QColor& color)> func) {
    setHideAfterLeave(false);

    PopupWidget* colorSelectWidget = new PopupWidget(this, Qt::Popup);
    QHBoxLayout* hboxlayout = new QHBoxLayout();
    QColorDialog* colorDiralog = new QColorDialog();

    QWidget::connect(colorDiralog, &QColorDialog::colorSelected, this, [colorButton, colorSelectWidget, func, this](const QColor& color) {
            setColorForButton(colorButton, color);
            colorSelectWidget->hide();
            func(color);
            setHideAfterLeave(true);
    });
    QWidget::connect(colorDiralog, &QColorDialog::rejected, colorSelectWidget, &PopupWidget::hide);
    QWidget::connect(colorSelectWidget, &PopupWidget::hided, colorSelectWidget, [colorSelectWidget]() {
            colorSelectWidget->deleteLater();
    });

    hboxlayout->addWidget(colorDiralog);
    hboxlayout->setContentsMargins(0, 0, 0, 0);
    colorSelectWidget->setLayout(hboxlayout);
    colorSelectWidget->setAssociateWidget(colorButton);
    colorSelectWidget->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    colorSelectWidget->setAuotLayout();
    colorSelectWidget->show();
}

void FullSettingWidget::setColorForButton(QPushButton* colorButton,const QColor &color) {
    colorButton->setText(color.name());
    colorButton->setStyleSheet("QPushButton{background-color: " + color.name() + ";"
        + "color: " + color.darker().name() + ";"
        + R"(border: 1px dashed white;})");
}

void FullSettingWidget::refresh() {
    d->update();
}
