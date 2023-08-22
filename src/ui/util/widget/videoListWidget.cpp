#include "videoListWidget.hpp"

const QString kScrollBarStyleSheet =
R"(
QScrollBar:horizontal {
    height: 10px;
    border: 1px solid grey;
    border-radius: 5px;
    background: #ffffff;
    margin: 0px 0px 0px 0px;
}
QScrollBar::handle:horizontal {
    background: #696969;
    min-width: 20px;
}
QScrollBar::add-line:horizontal {
    width: 0px;
}
QScrollBar::sub-line:horizontal {
    width: 0px;
}
)";

VideoListWidget::VideoListWidget(QWidget* parent) : QListWidget(parent) { 
    setFlow(QListView::LeftToRight);
    setStyleSheet(kScrollBarStyleSheet);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

VideoListWidget::~VideoListWidget() {}

QSize VideoListWidget::sizeHint() const {
    int width = 0, height = 0;
    for (int i = 0; i < count(); i++) {
        QListWidgetItem* item = this->item(i);
        auto wi = itemWidget(item);
        QSize itemSize = wi->sizeHint();
        width += itemSize.width();
        height = std::max(height, itemSize.height());
    }
    return QSize(width, height);
}

void VideoListWidget::addWidgetItem(QWidget *item)  {
    QListWidgetItem* listItem = new QListWidgetItem(this);
    // TODO(llhsdmd) : item widget 的 size 在加入后正式更新前会更新自己的size，此时的size不一定正确。
    listItem->setSizeHint(item->sizeHint() + QSize(10, 0));
    qDebug() << "VideoListWidget::addWidgetItem : " << item->sizeHint();
    setItemWidget(listItem, item);
    listItem->setTextAlignment(Qt::AlignCenter);
}
