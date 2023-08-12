#include "popupWidget.hpp"

#include <QApplication>
#include <QScreen>
#include <QMouseEvent>

#include "../../../common/myGlobalLog.hpp"

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
                if (self->outside()) {
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
                if (self->outside()) {
                    return r1;
                }
                return r1 - r2 + l2;
            }
            return l2;
        }
        int layoutV(int t1, int b1, int t2, int b2, Qt::Alignment align) {
            if (align & Qt::AlignTop) {
                if (self->outside()) {
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
                if (self->outside()) {
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
    mTimer = new QTimer(this);

    mTimer->setSingleShot(true);
    connect(mTimer, &QTimer::timeout, this, &PopupWidget::hide, Qt::UniqueConnection);
    connect(this, &PopupWidget::showed, this, QOverload<>::of(&PopupWidget::stopHideTimer), Qt::UniqueConnection);
    setHidden(true);
    setMouseTracking(true);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

PopupWidget::~PopupWidget() { }

void PopupWidget::enterEvent(QEnterEvent* event) {
    Q_EMIT mouseEnter();
    if (mStopTimerEnter) {
        stopHideTimer();
    }
    QWidget::enterEvent(event);
}

void PopupWidget::hideEvent(QHideEvent *event) {
    Q_EMIT hided();
    QWidget::hideEvent(event);
}

void PopupWidget::focusInEvent(QFocusEvent *event) {
    Q_EMIT focusEnter();
    if (mStopTimerEnter) {
        stopHideTimer();
    }
    QWidget::focusInEvent(event);
}

void PopupWidget::focusOutEvent(QFocusEvent *event) {
    Q_EMIT focusLeave();
    if (mHideAfterLeave) {
        hideLater();
    }
    QWidget::focusOutEvent(event);
}

void PopupWidget::showEvent(QShowEvent *event) {
    if (mAutoLayout) {
        // 计算容器（父窗口或屏幕），贴靠对象，自身在容器坐标系下的矩阵
        QRect containerRect, selfRect;
        auto p = isWindow() ? nullptr : parentWidget();
        QPoint topLeft(0, 0);
        if (p != nullptr) {
            containerRect = p->rect();
            selfRect = rect();
            if (mAttachWidget != nullptr) {
                topLeft = p->mapFromGlobal(d->doLayout(mAttachWidget, mAligns));
            } else {
                topLeft = p->mapFromGlobal(d->doLayout(p, mAligns));
            }
        } else {
            containerRect = QApplication::primaryScreen()->availableGeometry();
            selfRect = QRect(parentWidget()->mapToGlobal(QPoint(0, 0)), size());
            if (mAttachWidget != nullptr) {
                topLeft = d->doLayout(mAttachWidget, mAligns);
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
    Q_EMIT showed();
    QWidget::showEvent(event);
}

bool PopupWidget::eventFilter(QObject* obj, QEvent* event) {
    if (obj == parent() && event->type() == QEvent::MouseButtonPress) {
        if (!rect().contains(static_cast<QMouseEvent*>(event)->pos())) {
            LOG(DEBUG) << "mouse button press outside";
            hideLater(100);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PopupWidget::mouseMoveEvent(QMouseEvent* event) {
    if (mStopTimerEnter) {
        stopHideTimer();
    }

    QWidget::mouseMoveEvent(event);
}

void PopupWidget::leaveEvent(QEvent* event) {
    Q_EMIT mouseLeave();
    if (mHideAfterLeave) {
        hideLater();
    }
    QWidget::leaveEvent(event);
}

void PopupWidget::hideLater(int msec) {
    msec = (msec == -1 ? mDefualtHideAfterTime : msec);
    mTimer->start(msec);
}

void PopupWidget::stopHideTimer() {
    if (mTimer->isActive()) {
        mTimer->stop();
    }
}

void PopupWidget::hideLater() {
    hideLater(-1);
}
