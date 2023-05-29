#pragma once

#include<QWidget>
#include <QScrollArea>

#include "videoView.hpp"

class HomeWidget : public QScrollArea {
    Q_OBJECT
    public:
    enum DisplayArea {
        New,
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
        Sunday,
        Recommend,
    };
    public:
        HomeWidget(QWidget* parent = nullptr);
        virtual ~HomeWidget() {}

        VideoView* addItem(const DisplayArea& area);
        QList<VideoView *> addItems(const DisplayArea& area, int count);
    
    public:
        void resizeEvent(QResizeEvent *) override;
        bool eventFilter(QObject *watched, QEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void mouseDoubleClickEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
        bool viewportEvent(QEvent *event) override;

    Q_SIGNALS:
        void refreshRequest(const DisplayArea area);
        void dataRequest(const DisplayArea area);

    public Q_SLOTS:
        void refresh(const QList<videoData>& dataList,const DisplayArea area);

    private:
        void _refresh(QWidget* container, const QList<videoData>& dataList);
        QList<VideoView *> _addItems(QWidget* container, int count);
        void _setupUi();

    private:
        void *ui;
        QWidget* contents;
};