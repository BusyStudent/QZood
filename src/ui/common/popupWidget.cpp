#include "popupWidget.hpp"

#include <QApplication>
#include <QScreen>
#include <QMouseEvent>

PopupWidget::PopupWidget(QWidget* parent, Qt::WindowFlags f) : QWidget(parent, Qt::Popup) {
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &PopupWidget::hide);
    hide();
}

void PopupWidget::enterEvent(QEnterEvent* event) {
    stopHideTimer();
    QWidget::enterEvent(event);
}

void PopupWidget::hideEvent(QHideEvent *event) {
    stopHideTimer();
    emit hided();
    QWidget::hideEvent(event);
}

void PopupWidget::showEvent(QShowEvent *event) {
    stopHideTimer();
    if (auto_layout && attach_widget != nullptr) {
        // 计算容器（父窗口或屏幕），贴靠对象，自身在容器坐标系下的矩阵
        QRect containerRect, attachWidgetGeometry, selfRect;
        QWidget* p = isWindow() ? nullptr : parentWidget();
        if (p != nullptr) {
            containerRect = p->rect();
            attachWidgetGeometry = QRect(attach_widget->mapTo(p, QPoint(0,0)), attach_widget->size());
            selfRect = rect();
        } else {
            containerRect = QApplication::primaryScreen()->availableGeometry();
            attachWidgetGeometry = QRect(attach_widget->mapToGlobal(QPoint(0,0)), attach_widget->size());
            selfRect = QRect(QPoint(0, 0), size());
        }
        // 根据所贴的边计算自身理想状态下左上角的坐标
        QPoint topLeft(0, 0);
        if (direction == TOP) {
            topLeft = attachWidgetGeometry.topLeft() - 
                QPoint((selfRect.width() - attachWidgetGeometry.width()) / 2, selfRect.height());
        } else if (direction == LEFT) {
            topLeft = attachWidgetGeometry.topLeft() - 
                QPoint(selfRect.width(), (selfRect.height() - attachWidgetGeometry.height()) / 2);
        } else if (direction == BOTTOM) {
            topLeft = attachWidgetGeometry.topLeft() + 
                QPoint((selfRect.width() - attachWidgetGeometry.width()) / 2, attachWidgetGeometry.height());
        } else if (direction == RIGHT) {
            topLeft = attachWidgetGeometry.topLeft() - 
                QPoint(attachWidgetGeometry.width(), (selfRect.height() - attachWidgetGeometry.height()) / 2);
        }

        // 根据自身超出容器范围进行位置调整
        if (topLeft.y() + selfRect.height() > containerRect.height()) {
            topLeft.setY(containerRect.height() - selfRect.height());
        }
        if (topLeft.y() < 0) {
            topLeft.setY(0);
        }
        if (topLeft.x() + selfRect.width() > containerRect.width()) {
            topLeft.setX(containerRect.width() - selfRect.width());
        }
        if (topLeft.x() < 0) {
            topLeft.setX(0);
        }

        move(topLeft);
    }
    emit showed();
    QWidget::showEvent(event);
}

bool PopupWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == parent() && event->type() == QEvent::MouseButtonPress) {
        if (!rect().contains(static_cast<QMouseEvent*>(event)->pos())) {
            qDebug() << "mouse button press outside";
            hideLater(100);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PopupWidget::leaveEvent(QEvent* event) {
    if (hide_after_leave) {
        hideLater();
    }
    QWidget::leaveEvent(event);
}

void PopupWidget::hideLater(int msec) {
    msec = msec == -1 ? defualt_hide_after_time : msec;
    timer->start(msec);
}

void PopupWidget::stopHideTimer() {
    if (timer->isActive()) {
        timer->stop();
    }
}