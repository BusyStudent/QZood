#include "customizeTitleWidget.hpp"

#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHoverEvent>
#include <QPainter>

#if defined(_WIN32)
#define USE_WINAPI
#endif

#if defined(USE_WINAPI)
#include <dwmapi.h>
#include <qt_windows.h>
#include <windowsx.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#endif

CustomizeTitleWidget::CustomizeTitleWidget(QWidget *parent) : QWidget(parent) {
  setWindowFlags(Qt::FramelessWindowHint);     // 隐藏窗体原始边框
  setAttribute(Qt::WA_StyledBackground);       // 启用样式背景绘制
  setAttribute(Qt::WA_TranslucentBackground);  // 设置背景透明
  setAttribute(Qt::WA_Hover);                  // 启动鼠标悬浮追踪
}

bool CustomizeTitleWidget::nativeEvent(const QByteArray &eventType,
                                       void *message, qintptr *result) {
#if defined(USE_WINAPI)
  MSG *param = static_cast<MSG *>(message);
  switch (param->message) {
    case WM_NCCALCSIZE: {
      /*
      此消息用于处理非客户区域，比如边框的绘制
      返回false,就是按系统的默认处理，如果返回true,而不做任何绘制，则非客户区域
      就不会被绘制，就相当于没有绘制非客户区域，所以就会看不到非客户区域的效果
      */
      return true;
    } case WM_NCHITTEST: {
      RECT winrect;
      GetWindowRect(HWND(winId()), &winrect);

      long x = GET_X_LPARAM(param->lParam);
      long y = GET_Y_LPARAM(param->lParam);

      /*
      只用这种办法设置动态改变窗口大小比手动通过鼠标事件效果好，可以
      避免闪烁问题
      */
      bool insideLeft = x > winrect.left - MARGIN_OUT_SIZE && x < winrect.left + MARGIN_IN_SIZE;
      bool insideRight = x > winrect.right - MARGIN_IN_SIZE && x < winrect.right + MARGIN_OUT_SIZE;
      bool insideTop = y > winrect.top - MARGIN_OUT_SIZE && y < winrect.top + MARGIN_IN_SIZE;
      bool insideBottom = y > winrect.bottom - MARGIN_IN_SIZE && y < winrect.bottom + MARGIN_OUT_SIZE;
      // left border
      if (insideLeft) {
        *result = HTLEFT;
        return true;
      }
      // right border
      if (insideRight) {
        *result = HTRIGHT;
        return true;
      }
      // top border
      if (insideTop) {
        *result = HTTOP;
        return true;
      }
      // bottom border
      if (insideBottom) {
        *result = HTBOTTOM;
        return true;
      }
      // bottom left corner
      if (insideBottom && insideLeft) {
        *result = HTBOTTOMLEFT;
        return true;
      }
      // bottom right corner
      if (insideBottom && insideRight) {
        *result = HTBOTTOMRIGHT;
        return true;
      }
      // top left corner
      if (insideTop && insideLeft) {
        *result = HTTOPLEFT;
        return true;
      }
      // top right corner
      if (insideTop && insideRight) {
        *result = HTTOPRIGHT;
        return true;
      }

      return QWidget::nativeEvent(eventType, message, result);
    }
  }
#endif
  return QWidget::nativeEvent(eventType, message, result);
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
#ifndef USE_WINAPI
  if (windowState() != Qt::WindowMaximized && !movingStatus()) {
      updateRegion(event);
  }
#endif
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
    if ((marginRight >= -MARGIN_OUT_SIZE && marginRight <= MARGIN_IN_SIZE) &&
        ((marginBottom <= MARGIN_IN_SIZE) &&
         marginBottom >= -MARGIN_OUT_SIZE)) {
      m_direction = BOTTOMRIGHT;
      setCursor(Qt::SizeFDiagCursor);
    } else if ((marginTop >= -MARGIN_OUT_SIZE && marginTop <= MARGIN_IN_SIZE) &&
               (marginRight >= -MARGIN_OUT_SIZE &&
                marginRight <= MARGIN_IN_SIZE)) {
      m_direction = TOPRIGHT;
      setCursor(Qt::SizeBDiagCursor);
    } else if ((marginLeft >= -MARGIN_OUT_SIZE &&
                marginLeft <= MARGIN_IN_SIZE) &&
               (marginTop >= -MARGIN_OUT_SIZE && marginTop <= MARGIN_IN_SIZE)) {
      m_direction = TOPLEFT;
      setCursor(Qt::SizeFDiagCursor);
    } else if ((marginLeft >= -MARGIN_OUT_SIZE &&
                marginLeft <= MARGIN_IN_SIZE) &&
               (marginBottom >= -MARGIN_OUT_SIZE &&
                marginBottom <= MARGIN_IN_SIZE)) {
      m_direction = BOTTOMLEFT;
      setCursor(Qt::SizeBDiagCursor);
    } else if (marginBottom <= MARGIN_IN_SIZE &&
               marginBottom >= -MARGIN_OUT_SIZE) {
      m_direction = DOWN;
      setCursor(Qt::SizeVerCursor);
    } else if (marginLeft <= MARGIN_IN_SIZE && marginLeft >= -MARGIN_OUT_SIZE) {
      m_direction = LEFT;
      setCursor(Qt::SizeHorCursor);
    } else if (marginRight <= MARGIN_IN_SIZE &&
               marginRight >= -MARGIN_OUT_SIZE) {
      m_direction = RIGHT;
      setCursor(Qt::SizeHorCursor);
    } else if (marginTop <= MARGIN_IN_SIZE && marginTop >= -MARGIN_OUT_SIZE) {
      m_direction = UP;
      setCursor(Qt::SizeVerCursor);
    } else {
      setCursor(Qt::ArrowCursor);
    }
  }

  flag_resizing = flag_pressed ? flag_resizing : (NONE != m_direction);

  if (resizingStatus()) {
    resizeRegion(marginTop, marginBottom, marginLeft, marginRight);
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
//   layout()->setContentsMargins(0, 0, 0, 0);
  QWidget::showMaximized();
}
void CustomizeTitleWidget::showFullScreen() {
//   layout()->setContentsMargins(0, 0, 0, 0);
  QWidget::showFullScreen();
}
void CustomizeTitleWidget::showNormal() {
//   layout()->setContentsMargins(0, 0, 0, 0);
  QWidget::showNormal();
}

void CustomizeTitleWidget::paintEvent(QPaintEvent *event) {
    //TODO(llhsdmd): 绘制阴影

  QWidget::paintEvent(event);
}

void CustomizeTitleWidget::createShadow(QWidget *container_widget) {
    containerWidget = container_widget;
    layout()->setContentsMargins(0, 0, 0, 0);
#ifdef USE_WINAPI
    BOOL composition = FALSE;
    if (FAILED(DwmIsCompositionEnabled(&composition)) || !composition) {
        return ;
    }
    DWMNCRENDERINGPOLICY policy = DWMNCRP_ENABLED;
    HRESULT hr;
    hr = DwmSetWindowAttribute(HWND(winId()), DWMWA_NCRENDERING_POLICY, &policy, sizeof(policy));
    if (FAILED(hr)) {
        return ;
    }
    MARGINS m{0};
    m = MARGINS {4, 4, 4, 4};
    hr = DwmExtendFrameIntoClientArea(HWND(winId()), &m);
#endif
}