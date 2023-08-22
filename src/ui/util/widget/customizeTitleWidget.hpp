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

  virtual bool isInTitleBar(const QPoint &pos) = 0;

 public:
  bool event(QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

 public:
  void showMinimized();
  void showMaximized();
  void showFullScreen();
  void showNormal();
  inline bool resizingStatus() { return flag_resizing && flag_pressed; }
  inline bool movingStatus() { return flag_moving && flag_pressed; }
  inline void setResizingStatus(bool v) { flag_resizing = v; }
  inline void setMovingStatus(bool v) { flag_moving = v; }

 private:
  const int MARGIN_IN_SIZE = 5;
  const int MARGIN_OUT_SIZE = 5;
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
  QPoint diff_pos;
  bool flag_resizing = false;
  bool flag_pressed = false;
  bool flag_moving = false;
  QWidget* containerWidget = nullptr;
};