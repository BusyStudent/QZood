#include "tabListWidget.hpp"

#include <QWidget>
#include <QDebug>

TabListWidget::TabListWidget(QWidget* parent) : QListWidget(parent) {}

TabListWidget::~TabListWidget() {}

QSize TabListWidget::sizeHint() const {
    int width = 0, height = 0;
    for (int i = 0; i < count(); ++i){
        QListWidgetItem* item = this->item(i);
        // 根据item的内容设置大小, 获取Item内容的size
        QString itemText = item->text();
        QFontMetrics fm(item->font());
        auto boundRect = fm.boundingRect(itemText);

        width += boundRect.width() + 8;
        height = qMax(height, boundRect.height());
    }
    width += 6;
    return QSize(width, height + 3);
}