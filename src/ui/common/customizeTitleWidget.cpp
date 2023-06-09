#include "customizeTitleWidget.hpp"

#include <QEvent>
#include <QHoverEvent>
#include <QGraphicsDropShadowEffect>

CustomizeTitleWidget::CustomizeTitleWidget(QWidget *parent) : QWidget(parent) {
  setWindowFlags(Qt::FramelessWindowHint);     // 隐藏窗体原始边框
  setAttribute(Qt::WA_StyledBackground);       // 启用样式背景绘制
  setAttribute(Qt::WA_TranslucentBackground);  // 设置背景透明
  setAttribute(Qt::WA_Hover);                  // 启动鼠标悬浮追踪
}

bool CustomizeTitleWidget::event(QEvent *event) {
  if (event->type() == QEvent::HoverMove) {
    QHoverEvent *hoverEvent = static_cast<QHoverEvent *>(event);
    QMouseEvent mouseEvent(QEvent::MouseMove, hoverEvent->pos(), Qt::NoButton,
                           Qt::NoButton, Qt::NoModifier);
    mouseMoveEvent(&mouseEvent);
  }

  return QWidget::event(event);
}

void CustomizeTitleWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    flag_pressed = true;
    press_pos = event->globalPos();
    diff_pos = press_pos - window()->pos();
  }

  QWidget::mousePressEvent(event);
}

void CustomizeTitleWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }

    QWidget::mouseDoubleClickEvent(event);
}

void CustomizeTitleWidget::mouseMoveEvent(QMouseEvent *event) {
    if (flag_pressed) {
        move_pos = event->globalPos() - press_pos;
        press_pos += move_pos;
    }

    if (windowState() != Qt::WindowMaximized && !movingStatus()) {
        updateRegion(event);
    }

    flag_moving = !resizingStatus();

    QWidget::mouseMoveEvent(event);
}

void CustomizeTitleWidget::mouseReleaseEvent(QMouseEvent *event) {
    flag_pressed = false;
    flag_resizing = false;
    flag_moving = false;
    setCursor(Qt::ArrowCursor);

    QWidget::mouseReleaseEvent(event);
}

void CustomizeTitleWidget::leaveEvent(QEvent *event) {
  flag_pressed = false;
  flag_resizing = false;
  flag_moving = false;
  setCursor(Qt::ArrowCursor);

  QWidget::leaveEvent(event);
}

void CustomizeTitleWidget::updateRegion(QMouseEvent *event) {
    QRect mainRect = window()->geometry();

    int marginTop = event->globalY() - mainRect.y();
    int marginBottom = mainRect.y() + mainRect.height() - event->globalY();
    int marginLeft = event->globalX() - mainRect.x();
    int marginRight = mainRect.x() + mainRect.width() - event->globalX();

    if (!resizingStatus()) {
        m_direction = NONE;
        if ((marginRight >= MARGIN_MIN_SIZE && 
             marginRight <= MARGIN_MAX_SIZE) &&
           ((marginBottom <= MARGIN_MAX_SIZE) &&
             marginBottom >= MARGIN_MIN_SIZE)) {
            m_direction = BOTTOMRIGHT;
            setCursor(Qt::SizeFDiagCursor);
        } else if ((marginTop >= MARGIN_MIN_SIZE && 
                    marginTop <= MARGIN_MAX_SIZE) &&
                   (marginRight >= MARGIN_MIN_SIZE &&
                    marginRight <= MARGIN_MAX_SIZE)) {
            m_direction = TOPRIGHT;
            setCursor(Qt::SizeBDiagCursor);
        } else if ((marginLeft >= MARGIN_MIN_SIZE &&
                    marginLeft <= MARGIN_MAX_SIZE) &&
                   (marginTop >= MARGIN_MIN_SIZE && 
                    marginTop <= MARGIN_MAX_SIZE)) {
            m_direction = TOPLEFT;
            setCursor(Qt::SizeFDiagCursor);
        } else if ((marginLeft >= MARGIN_MIN_SIZE &&
                    marginLeft <= MARGIN_MAX_SIZE) &&
                   (marginBottom >= MARGIN_MIN_SIZE &&
                    marginBottom <= MARGIN_MAX_SIZE)) {
            m_direction = BOTTOMLEFT;
            setCursor(Qt::SizeBDiagCursor);
        } else if (marginBottom <= MARGIN_MAX_SIZE &&
                   marginBottom >= MARGIN_MIN_SIZE) {
            m_direction = DOWN;
            setCursor(Qt::SizeVerCursor);
        } else if (marginLeft <= MARGIN_MAX_SIZE &&
                   marginLeft >= MARGIN_MIN_SIZE) {
            m_direction = LEFT;
            setCursor(Qt::SizeHorCursor);
        } else if (marginRight <= MARGIN_MAX_SIZE &&
                   marginRight >= MARGIN_MIN_SIZE) {
            m_direction = RIGHT;
            setCursor(Qt::SizeHorCursor);
        } else if (marginTop <= MARGIN_MAX_SIZE && 
                   marginTop >= MARGIN_MIN_SIZE) {
            m_direction = UP;
            setCursor(Qt::SizeVerCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }

    flag_resizing = flag_pressed ? flag_resizing : (NONE != m_direction);

    if (resizingStatus()) {
        resizeRegion(marginTop, marginBottom, marginLeft, marginRight);
        event->accept();
    }
}

void CustomizeTitleWidget::resizeRegion(int marginTop, int marginBottom,
                                        int marginLeft, int marginRight) {
    if (NONE == m_direction) {
        return;
    }
    QRect rect = frameGeometry();
    switch (m_direction) {
        case BOTTOMRIGHT: {
            rect.setBottomRight(rect.bottomRight() + move_pos);
        } break;
        case TOPRIGHT: {
        if (marginLeft > minimumWidth() && marginBottom > minimumHeight()) {
            rect.setTopRight(rect.topRight() + move_pos);
        }
        } break;
        case TOPLEFT: {
        if (marginRight > minimumWidth() && marginBottom > minimumHeight()) {
            rect.setTopLeft(rect.topLeft() + move_pos);
        }
        } break;
        case BOTTOMLEFT: {
        if (marginRight > minimumWidth() && marginTop > minimumHeight()) {
            rect.setBottomLeft(rect.bottomLeft() + move_pos);
        }
        } break;
        case RIGHT: {
            rect.setWidth(rect.width() + move_pos.x());
        } break;
        case DOWN: {
            rect.setHeight(rect.height() + move_pos.y());
        } break;
        case LEFT: {
        if (marginRight > minimumWidth()) {
            rect.setLeft(rect.x() + move_pos.x());
        }
        } break;
        case UP: {
        if (marginBottom > minimumHeight()) {
            rect.setTop(rect.y() + move_pos.y());
        }
        } break;
    }
    setGeometry(rect);

}
void CustomizeTitleWidget::showMinimized() { QWidget::showMinimized(); }
void CustomizeTitleWidget::showMaximized() {
    layout()->setContentsMargins(0, 0, 0, 0);
    QWidget::showMaximized();
}
void CustomizeTitleWidget::showFullScreen() { 
    layout()->setContentsMargins(0, 0, 0, 0);
    QWidget::showFullScreen();
}
void CustomizeTitleWidget::showNormal() {
    layout()->setContentsMargins(5, 5, 5, 5);
    QWidget::showNormal();
}

void CustomizeTitleWidget::createShadow(QWidget* container_widget) {
  QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
  shadowEffect->setColor(QColor(100, 100, 100));
  shadowEffect->setOffset(0, 0);
  shadowEffect->setBlurRadius(13);
  container_widget->setGraphicsEffect(shadowEffect);
}