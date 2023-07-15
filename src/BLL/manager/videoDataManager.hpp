#pragma once

#include <QObject>

#include "../data/videoSourceBLL.hpp"

class VideoDataManager : public QObject {
    Q_OBJECT
    public:
        static VideoDataManager *instance();
        bool ready();
        bool preparetion();
        bool unprepared();
        bool updatable();
    
    public Q_SLOTS:
        void requestTimelineData();

    Q_SIGNALS:
        void timelineDataReply(const TimelineEpisodeList tep, int week);

    protected:
        VideoDataManager(QObject *parent = nullptr);

    private:
        QAtomicInteger<bool> mDataReady;
        QAtomicInteger<bool> mDataUpdatable;
};