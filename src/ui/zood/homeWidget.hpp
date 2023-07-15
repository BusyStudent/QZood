#pragma once

#include<QWidget>
#include <QScrollArea>

#include "videoView.hpp"
#include "../../BLL/data/videoBLL.hpp"

class HomeWidgetPrivate;

class HomeWidget : public QScrollArea {
    Q_OBJECT
    public:
    enum DisplayArea : int {
        Monday = 1,
        Tuesday = 2,
        Wednesday = 3,
        Thursday = 4,
        Friday = 5,
        Saturday = 6,
        Sunday = 7,
        Recommend = 8,
        New = 9,
    };
    public:
        HomeWidget(QWidget* parent = nullptr);
        virtual ~HomeWidget();

        VideoView* addItem(const DisplayArea& area);
        QList<VideoView *> addItems(const DisplayArea& area, int count);
        void clearItem(const DisplayArea& area);

    public Q_SLOTS :
        void runPlayer(VideoBLLList videos, const QString &title);
    
    public:
        void resizeEvent(QResizeEvent *) override;
        bool eventFilter(QObject *watched, QEvent *event) override;
        void mousePressEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void mouseDoubleClickEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;

    Q_SIGNALS:
        void refreshRequest();
        void dataRequest();

    public Q_SLOTS:
        void refresh(const VideoDataVector& dataVector,const DisplayArea area);
        void updateVideo(const VideoDataVector& dataVector,const DisplayArea area);

    private:
        QScopedPointer<HomeWidgetPrivate> d;
};