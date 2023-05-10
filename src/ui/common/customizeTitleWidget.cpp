#include "customizeTitleWidget.hpp"

#include <QGraphicsDropShadowEffect>
#include <QMenuBar>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QShowEvent>
#include <QStatusBar>
#include <QStyle>

TitleBar::TitleBar(QWidget *parent) : QWidget(parent) {
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

  hBoxLayout = new QHBoxLayout();

  setLayout(hBoxLayout);
  hBoxLayout->setContentsMargins(5, 10, 5, 10);
  hBoxLayout->setSpacing(2);
}
void TitleBar::MinimizedEvent(bool checked) {
  CustomizeTitleWidget *main_window =
      dynamic_cast<CustomizeTitleWidget *>(window());
  if (main_window != nullptr) {
    main_window->showMinimized();
  } else {
    window()->showMinimized();
  }
}
void TitleBar::MaximizedEvent(bool checked) {
  if (!flag_resizable) {
    return;
  }
  if (!window()->isMaximized()) {
    showMaximized();
  } else {
    showNormal();
  }
}
void TitleBar::showMaximized() {
  CustomizeTitleWidget *main_window =
      dynamic_cast<CustomizeTitleWidget *>(window());
  if (main_window != nullptr) {
    main_window->showMaximized();
  } else {
    window()->showMaximized();
  }
}

void TitleBar::showFullLeftScreen() {
  if (!flag_welted && flag_resizable) {
    flag_welted = true;
    normal_size = window()->size();
    auto screen_size = window()->screen()->availableSize();
    window()->resize(screen_size.width() / 2, screen_size.height());
    window()->move(0, 0);
  }
}
void TitleBar::showFullRightScreen() {
  if (!flag_welted && flag_resizable) {
    flag_welted = true;
    normal_size = window()->size();
    auto screen_size = window()->screen()->availableSize();
    window()->resize(screen_size.width() / 2, screen_size.height());
    window()->move(screen_size.width() / 2, 0);
  }
}
void TitleBar::showNormal() {
  if (flag_welted) {
    flag_welted = false;
    window()->resize(normal_size);
  } else {
    CustomizeTitleWidget *main_window =
        dynamic_cast<CustomizeTitleWidget *>(window());
    if (main_window != nullptr) {
      main_window->showNormal();
    } else {
      window()->showNormal();
    }
  }
}

void TitleBar::CloseEvent(bool checked) { window()->close(); }

void TitleBar::setResizable(bool resizeable) {
  flag_resizable = resizeable;
}

void TitleBar::setMovable(bool moveable) { flag_movable = moveable; }

void TitleBar::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && flag_movable && !flag_doubleClick) {
    flag_moving = true;
    diff_position = window()->pos() - event->globalPos();
  }
  QWidget::mousePressEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    flag_moving = false;
  }
  flag_doubleClick = false;
  QWidget::mouseReleaseEvent(event);
}

void TitleBar::leaveEvent(QEvent *event) {
  flag_moving = false;
  QWidget::leaveEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event) {
  if (flag_moving && flag_movable) {
    auto screen_size = window()->screen()->availableSize();
    if ((window()->isMaximized() && event->globalY() >= 10) ||
        (flag_welted && (event->globalX() >= 5 &&
                         event->globalX() <= screen_size.width() - 5))) {
      showNormal();
      diff_position.setX(1);
    } else {
      if (diff_position.x() == 1) {
        diff_position.setX(-window()->size().width() / 2);
      }
      if (event->globalY() < 10) {
        if (!window()->isMaximized()) {
          MaximizedEvent(true);
        }
      } else if (event->globalX() < 5) {
        showFullLeftScreen();
      } else if (screen_size.width() - event->globalX() < 5) {
        showFullRightScreen();
      } else {
        window()->move(event->globalPos() + diff_position);
      }
    }
  }
  QWidget::mouseMoveEvent(event);
}
void TitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    flag_doubleClick = true;
    MaximizedEvent(true);
  }
  QWidget::mouseDoubleClickEvent(event);
}

void TitleBar::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  auto self_rect = rect();
  painter.setPen(QColor(100, 100, 100));
  painter.drawLine(0, self_rect.height() - 1, self_rect.width(),
                   self_rect.height() - 1);
  QWidget::paintEvent(event);
}

CustomizeTitleWidget::CustomizeTitleWidget(QWidget *parent) : QWidget(parent) {
  setWindowFlags(Qt::FramelessWindowHint);     // 隐藏窗体原始边框
  setAttribute(Qt::WA_StyledBackground);       // 启用样式背景绘制
  setAttribute(Qt::WA_TranslucentBackground);  // 设置背景透明
  setAttribute(Qt::WA_Hover);                  // 启动鼠标悬浮追踪

  title_bar = new TitleBar();

  container_widget = new QWidget();
  container_widget->setObjectName("containerWidget");
  container_layout = new QVBoxLayout();
  container_widget->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Expanding);
  container_layout->addWidget(container_widget);
  container_layout->setMargin(5);
  container_layout->setSpacing(0);
  setLayout(container_layout);

  main_layout = new QVBoxLayout();
  container_widget->setLayout(main_layout);
  main_layout->setMargin(0);
  main_layout->setSpacing(0);

  top_layout = new QVBoxLayout();
  center_layout = new QVBoxLayout();
  bottom_layout = new QVBoxLayout();

  main_layout->addWidget(title_bar);
  main_layout->addLayout(top_layout);
  main_layout->addLayout(center_layout, 1);
  main_layout->addLayout(bottom_layout);

  center_layout->addWidget(new QWidget());
  setStyleSheet(
      "QWidget#containerWidget{background-color: #ffc0cb}"
      "QWidget#containerWidget{border: 1px solid rgb(200,200,200)}"
      "QWidget#containerWidget{border-radius: 5px}");

  createShadow();
}

void CustomizeTitleWidget::addToolBar(QToolBar *toolBar) {
  top_layout->addWidget(toolBar);
}
void CustomizeTitleWidget::addMenuBar(QMenuBar *menu) {
  title_bar->insertWidget(menu, 1);
}
void CustomizeTitleWidget::setCentralWidget(QWidget *centralWidget) {
  if (!center_layout->isEmpty()) {
    QLayoutItem *child;
    while ((child = center_layout->takeAt(0)) != 0) {
      if (child->widget()) {
        child->widget()->setParent(NULL);
      }
      delete child;
    }
  }
  center_layout->addWidget(centralWidget);
}

void CustomizeTitleWidget::setStatusBar(QStatusBar *statusBar) {
  bottom_layout->addWidget(statusBar);
}

void CustomizeTitleWidget::paintEvent(QPaintEvent *event) {
  QWidget::paintEvent(event);
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

void CustomizeTitleWidget::insertActionToTitleBar(QAction *action, int index) {
  QToolButton *button = new QToolButton();
  button->setDefaultAction(action);
  title_bar->insertWidget(button, index);
}

void CustomizeTitleWidget::mouseMoveEvent(QMouseEvent *event) {
  if (flag_pressed) {
    move_pos = event->globalPos() - press_pos;
    press_pos += move_pos;
  }

  if (windowState() != Qt::WindowMaximized) {
    updateRegion(event);
  }

  if (!flag_resizing) {
    flag_pressed = false;
  }
  QWidget::mouseMoveEvent(event);
}

void CustomizeTitleWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    flag_pressed = false;
    flag_resizing = false;
    title_bar->setMovable(true);
    setCursor(Qt::ArrowCursor);
  }

  QWidget::mouseReleaseEvent(event);
}

void CustomizeTitleWidget::leaveEvent(QEvent *event) {
  flag_pressed = false;
  flag_resizing = false;
  title_bar->setMovable(true);
  setCursor(Qt::ArrowCursor);

  QWidget::leaveEvent(event);
}

void CustomizeTitleWidget::updateRegion(QMouseEvent *event) {
  QRect mainRect = geometry();

  int marginTop = event->globalY() - mainRect.y();
  int marginBottom = mainRect.y() + mainRect.height() - event->globalY();
  int marginLeft = event->globalX() - mainRect.x();
  int marginRight = mainRect.x() + mainRect.width() - event->globalX();

  if (!flag_resizing && flag_resizable) {
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
    title_bar->setMovable(false);
    resizeRegion(marginTop, marginBottom, marginLeft, marginRight);
  }
}
void CustomizeTitleWidget::resizeRegion(int marginTop, int marginBottom,
                                        int marginLeft, int marginRight) {
  if (flag_pressed) {
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
    title_bar->setMovable(true);
    m_direction = NONE;
  }
}
void CustomizeTitleWidget::showMinimized() { QWidget::showMinimized(); }
void CustomizeTitleWidget::showMaximized() {
  container_layout->setMargin(0);
  QWidget::showMaximized();
}
void CustomizeTitleWidget::showFullScreen() { QWidget::showFullScreen(); }
void CustomizeTitleWidget::showNormal() {
  container_layout->setMargin(5);
  QWidget::showNormal();
}

void CustomizeTitleWidget::createShadow() {
  QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect(this);
  shadowEffect->setColor(QColor(100, 100, 100));
  shadowEffect->setOffset(0, 0);
  shadowEffect->setBlurRadius(13);
  container_widget->setGraphicsEffect(shadowEffect);
}

void CustomizeTitleWidget::setWindowResizable(bool resizable) {
  flag_resizable = resizable;
  title_bar->setResizable(resizable);
}