#include "customizeTitleWidget.hpp"

#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHoverEvent>
#include <QPainter>
#include <iostream>

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
#if !defined(USE_WINAPI)
  setWindowFlags(Qt::FramelessWindowHint);     // 隐藏窗体原始边框
#endif
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
      *result = 0;
      return true;
    } case WM_NCHITTEST: {
    
      auto dpi = ::GetDpiForWindow(HWND(winId()));
      
      const POINT border {
        ::GetSystemMetricsForDpi(SM_CXFRAME, dpi) + ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi),
        ::GetSystemMetricsForDpi(SM_CYFRAME, dpi) + ::GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi)
      };
      RECT winrect;
      ::GetWindowRect(HWND(winId()), &winrect);

      long x = GET_X_LPARAM(param->lParam);
      long y = GET_Y_LPARAM(param->lParam);
      POINT userPos {x, y};
      ::ScreenToClient(HWND(winId()), &userPos);

      /*
      只用这种办法设置动态改变窗口大小比手动通过鼠标事件效果好，可以
      避免闪烁问题
      */
      bool insideLeft = x < winrect.left + border.x;
      bool insideRight = x >= winrect.right - border.x;
      bool insideTop = y < winrect.top + border.y;
      bool insideBottom = y >= winrect.bottom - border.y;

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

      // Check if  is caption
      // TODO(llhsdmd): 暴露或者抽一个接口判断目前的点是否按到了标题栏上
      userPos.x = ::MulDiv(userPos.x, 96, dpi);
      userPos.y = ::MulDiv(userPos.y, 96, dpi);
      if (isInTitleBar(QPoint{userPos.x, userPos.y})) {
        *result = HTCAPTION;
        return true;
      }
      *result = HTCLIENT;
      return true;
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
#ifndef USE_WINAPI
  if (event->button() == Qt::LeftButton) {
    flag_pressed = true;
    press_pos = event->globalPos();
    diff_pos = press_pos - window()->pos();
  }
#endif
  QWidget::mousePressEvent(event);
}

void CustomizeTitleWidget::mouseDoubleClickEvent(QMouseEvent *event) {
#ifndef USE_WINAPI
  if (isMaximized()) {
    showNormal();
  } else {
    showMaximized();
  }
#endif
  QWidget::mouseDoubleClickEvent(event);
}

void CustomizeTitleWidget::mouseMoveEvent(QMouseEvent *event) {
#ifndef USE_WINAPI
  if (flag_pressed) {
    move_pos = event->globalPos() - press_pos;
    press_pos += move_pos;
  }

  if (windowState() != Qt::WindowMaximized && !movingStatus()) {
      updateRegion(event);
  }
  
  if (movingStatus() && isInTitleBar(event->pos())) {
    window()->move(event->globalPos() - diff_pos);
  }

  flag_moving = !resizingStatus();
#endif
  QWidget::mouseMoveEvent(event);
}

void CustomizeTitleWidget::mouseReleaseEvent(QMouseEvent *event) {
#ifndef USE_WINAPI
  flag_pressed = false;
  flag_resizing = false;
  flag_moving = false;
  setCursor(Qt::ArrowCursor);
#endif
  QWidget::mouseReleaseEvent(event);
}

void CustomizeTitleWidget::leaveEvent(QEvent *event) {
#ifndef USE_WINAPI
  flag_pressed = false;
  flag_resizing = false;
  flag_moving = false;
  setCursor(Qt::ArrowCursor);
#endif
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

    // SetWindowLongPtrW(HWND(winId()), GWL_STYLE, WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX);
#endif
}