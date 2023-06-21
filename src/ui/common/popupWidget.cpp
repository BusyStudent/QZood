#include "popupWidget.hpp"

#include <QApplication>
#include <QScreen>
#include <QMouseEvent>

class PopupWidgetPrivate {
    public:
        PopupWidgetPrivate(PopupWidget* parent) : self(parent) {}
        /**
         * @brief 将(l2,r2)以对齐方式对齐到(l1,r1)线段上
         * 
         * @param l1 起点
         * @param r1 终点
         * @param l2 
         * @param r2 
         * @return int 对齐后的l2
         */
        int layoutH(int l1, int r1, int l2, int r2, Qt::Alignment align) {
            if (align & Qt::AlignLeft) {
                if (r1 - l1 < r2 - l2) {
                    return l1 - r2 + l2;
                }
                return l1;
            }
            if (align & Qt::AlignHCenter) {
                auto center = (l1 + r1) / 2.0;
                auto center1 = (l2 + r2) / 2.0;
                return l2 + center - center1;
            }
            if (align & Qt::AlignRight) {
                if (r1 - l1 < r2 - l2) {
                    return r1;
                }
                return r1 - r2 + l2;
            }
            return l2;
        }
        int layoutV(int t1, int b1, int t2, int b2, Qt::Alignment align) {
            if (align & Qt::AlignTop) {
                if (b2 - t2 > b1 - t1) {
                    return t1 - b2 + t2;
                }
                return t1;
            }
            if (align & Qt::AlignVCenter) {
                auto center = (t1 + b1) / 2.0;
                auto center1 = (t2 + b2) / 2.0;
                return t2 + center - center1;
            }
            if (align & Qt::AlignBottom) {
                if (b2 - t2 > b1 - t1) {
                    return b1;
                }
                return b1 - b2 + t2;
            }
            return b2;
        }

        QPoint doLayout(QWidget* attachWidget, Qt::Alignment aligns) {
            auto attachWidgetGeometry = QRect(attachWidget->mapToGlobal(QPoint(0,0)), attachWidget->size());
            auto selfRect = QRect(QPoint(0, 0), self->size());
            if (self->parentWidget() != nullptr) {
                selfRect = QRect(self->parentWidget()->mapToGlobal(QPoint(0, 0)), self->size());
            }

            int l1 = attachWidgetGeometry.x();
            int r1 = attachWidgetGeometry.x() + attachWidgetGeometry.width();
            int l2 = selfRect.x();
            int r2 = selfRect.x() + selfRect.width();
            int l = layoutH(l1, r1, l2, r2, aligns);

            int t1 = attachWidgetGeometry.y();
            int b1 = attachWidgetGeometry.y() + attachWidgetGeometry.height();
            int t2 = selfRect.y();
            int b2 = selfRect.y() + selfRect.height();
            int t = layoutV(t1, b1, t2, b2, aligns);

            return QPoint(l, t);
        }
    private:
        PopupWidget *self;
};


PopupWidget::PopupWidget(QWidget* parent, Qt::WindowFlags f) : QWidget(parent, f), d(new PopupWidgetPrivate(this)) {
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &PopupWidget::hide);
    hide();
}

PopupWidget::~PopupWidget() {
    delete d;
}

void PopupWidget::enterEvent(QEnterEvent* event) {
    if (stop_timer_enter) {
        stopHideTimer();
    }
    QWidget::enterEvent(event);
}

void PopupWidget::hideEvent(QHideEvent *event) {
    stopHideTimer();
    emit hided();
    QWidget::hideEvent(event);
}

void PopupWidget::showEvent(QShowEvent *event) {
    stopHideTimer();
    if (auto_layout) {
        // 计算容器（父窗口或屏幕），贴靠对象，自身在容器坐标系下的矩阵
        QRect containerRect, selfRect;
        QWidget* p = isWindow() ? nullptr : parentWidget();
        qDebug() << "parent : " << p;
        QPoint topLeft(0, 0);
        if (p != nullptr) {
            containerRect = p->rect();
            selfRect = rect();
            if (attach_widget != nullptr) {
                topLeft = p->mapFromGlobal(d->doLayout(attach_widget, aligns));
            } else {
                topLeft = p->mapFromGlobal(d->doLayout(p, aligns));
            }
        } else {
            containerRect = QApplication::primaryScreen()->availableGeometry();
            selfRect = QRect(parentWidget()->mapToGlobal(QPoint(0, 0)), size());
            if (attach_widget != nullptr) {
                topLeft = d->doLayout(attach_widget, aligns);
            }
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