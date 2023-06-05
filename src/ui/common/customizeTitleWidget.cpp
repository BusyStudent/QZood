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

  if (windowState() != Qt::WindowMaximized && !flag_moving) {
    updateRegion(event);
  }

  if (flag_pressed && !flag_resizing) {
    flag_moving = true;
    if (isMaximized()) {
        showNormal();
        move(cursor().pos() - QPoint{geometry().width(), 100} / 2);
    }
    move(mapToGlobal(move_pos));
  }
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
  QRect mainRect = geometry();

  int marginTop = event->globalY() - mainRect.y();
  int marginBottom = mainRect.y() + mainRect.height() - event->globalY();
  int marginLeft = event->globalX() - mainRect.x();
  int marginRight = mainRect.x() + mainRect.width() - event->globalX();

  if (!flag_resizing) {
    if ((marginRight >= MARGIN_MIN_SIZE && marginRight <= MARGIN_MAX_SIZE) &&
        ((marginBottom <= MARGIN_MAX_SIZE) &&
         marginBottom >= MARGIN_MIN_SIZE)) {
      m_direction = BOTTOMRIGHT;
      setCursor(Qt::SizeFDiagCursor);
    } else if ((marginTop >= MARGIN_MIN_SIZE && marginTop <= MARGIN_MAX_SIZE) &&
               (marginRight >= MARGIN_MIN_SIZE &&
                marginRight <= MARGIN_MAX_SIZE)) {
      m_direction = TOPRIGHT;
      setCursor(Qt::SizeBDiagCursor);
    } else if ((marginLeft >= MARGIN_MIN_SIZE &&
                marginLeft <= MARGIN_MAX_SIZE) &&
               (marginTop >= MARGIN_MIN_SIZE && marginTop <= MARGIN_MAX_SIZE)) {
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
    } else if (marginLeft <= MARGIN_MAX_SIZE - 1 &&
               marginLeft >= MARGIN_MIN_SIZE - 1) {
      m_direction = LEFT;
      setCursor(Qt::SizeHorCursor);
    } else if (marginRight <= MARGIN_MAX_SIZE &&
               marginRight >= MARGIN_MIN_SIZE) {
      m_direction = RIGHT;
      setCursor(Qt::SizeHorCursor);
    } else if (marginTop <= MARGIN_MAX_SIZE && marginTop >= MARGIN_MIN_SIZE) {
      m_direction = UP;
      setCursor(Qt::SizeVerCursor);
    } else {
      if (!flag_pressed) {
        setCursor(Qt::ArrowCursor);
      }
    }
  }

  if (NONE != m_direction) {
    flag_resizing = true;
    resizeRegion(marginTop, marginBottom, marginLeft, marginRight);
  }
}
void CustomizeTitleWidget::resizeRegion(int marginTop, int marginBottom,
                                        int marginLeft, int marginRight) {
  if (flag_pressed && flag_resizing) {
    switch (m_direction) {
      case BOTTOMRIGHT: {
        QRect rect = geometry();
        rect.setBottomRight(rect.bottomRight() + move_pos);
        setGeometry(rect);
      } break;
      case TOPRIGHT: {
        if (marginLeft > minimumWidth() && marginBottom > minimumHeight()) {
          QRect rect = geometry();
          rect.setTopRight(rect.topRight() + move_pos);
          setGeometry(rect);
        }
      } break;
      case TOPLEFT: {
        if (marginRight > minimumWidth() && marginBottom > minimumHeight()) {
          QRect rect = geometry();
          rect.setTopLeft(rect.topLeft() + move_pos);
          setGeometry(rect);
        }
      } break;
      case BOTTOMLEFT: {
        if (marginRight > minimumWidth() && marginTop > minimumHeight()) {
          QRect rect = geometry();
          rect.setBottomLeft(rect.bottomLeft() + move_pos);
          setGeometry(rect);
        }
      } break;
      case RIGHT: {
        QRect rect = geometry();
        rect.setWidth(rect.width() + move_pos.x());
        setGeometry(rect);
      } break;
      case DOWN: {
        QRect rect = geometry();
        rect.setHeight(rect.height() + move_pos.y());
        setGeometry(rect);
      } break;
      case LEFT: {
        if (marginRight > minimumWidth()) {
          QRect rect = geometry();
          rect.setLeft(rect.x() + move_pos.x());
          setGeometry(rect);
        }
      } break;
      case UP: {
        if (marginBottom > minimumHeight()) {
          QRect rect = geometry();
          rect.setTop(rect.y() + move_pos.y());
          setGeometry(rect);
        }
      } break;
      default: {
      } break;
    }
  } else {
    flag_resizing = false;
    m_direction = NONE;
  }
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