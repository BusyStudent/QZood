#pragma once

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>

class CustomizeTitleWidget : public QWidget {
  Q_OBJECT
 public:
  CustomizeTitleWidget(QWidget *parent = nullptr);
  void updateRegion(QMouseEvent *event);
  void resizeRegion(int marginTop, int marginBottom, int marginLeft,
                    int marginRight);
  void createShadow(QWidget* container_widget);

 public:
  bool event(QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

 public:
  void showMinimized();
  void showMaximized();
  void showFullScreen();
  void showNormal();

 private:
  const int MARGIN_MIN_SIZE = 0;
  const int MARGIN_MAX_SIZE = 10;
  enum MovingDirection {
    NONE,
    BOTTOMRIGHT,
    TOPRIGHT,
    TOPLEFT,
    BOTTOMLEFT,
    DOWN,
    LEFT,
    RIGHT,
    UP
  };

 private:
  MovingDirection m_direction = NONE;
  QPoint press_pos;
  QPoint move_pos;
  bool flag_resizing = false;
  bool flag_pressed = false;
  bool flag_moving = false;
};