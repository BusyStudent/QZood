#pragma once

#include <QListWidget>

class VideoListWidget : public QListWidget {
Q_OBJECT
public:
    VideoListWidget(QWidget* parent = nullptr);
    ~VideoListWidget();
    void addWidgetItem(QWidget *item);
    QSize sizeHint() const override;
};