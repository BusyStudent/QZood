#pragma once

#include<QWidget>
#include <QScrollArea>

struct videoData {
    int videoId;
    QString videoTitle;
    QString videoExtraInformation;
    QString videoSourceInformation;
    QImage image;
};

class VideoView : public QWidget {
    Q_OBJECT
    public:
        VideoView(QWidget* parent = nullptr);
        virtual ~VideoView() {}

    public Q_SLOTS:
        void setImage(const QImage& image);
        void setTitle(const QString& str);
        void setExtraInformation(const QString& str);
        void setSourceInformation(const QString& str);
    
    private:
        void *ui;
};

class HomeWidget : public QScrollArea {
    Q_OBJECT
    public:
    enum DisplayArea {
        TimeNew,
        TimeMonday,
        TimeTuesday,
        TimeWednesday,
        TimeThursday,
        TimeFriday,
        TimeSaturday,
        TimeSunday,
        TimeRecommend,
    };
    public:
        HomeWidget(QWidget* parent = nullptr);
        virtual ~HomeWidget() {}

        VideoView* addItem(const DisplayArea& area);
        QList<VideoView *> addItems(const DisplayArea& area, int count);
    
    public:
        void resizeEvent(QResizeEvent *) override;
        bool eventFilter(QObject *watched, QEvent *event) override;

    Q_SIGNALS:
        void refreshRequest(const DisplayArea area);

    public Q_SLOTS:
        void refresh(const QList<videoData>& dataList,const DisplayArea area);

    private:
        void _refresh(QWidget* container, const QList<videoData>& dataList);
        QList<VideoView *> _addItems(QWidget* container, int count);

    private:
        void *ui;
        QWidget* contents;
};