#include "videoDataManager.hpp"

#include <QTimer>

#include "../../common/myGlobalLog.hpp"

#ifndef VideoDebug(type)
#define VideoDebug(type) MDebug(type)
#endif

VideoDataManager *VideoDataManager::instance() {
    static VideoDataManager *mInstance = new VideoDataManager(qApp);
    return mInstance;
}
bool VideoDataManager::ready() {
    return (!mDataReady || mDataUpdatable);
}
bool VideoDataManager::preparetion() {
    return mDataReady;
}
bool VideoDataManager::unprepared() {
    return !mDataReady;
}
bool VideoDataManager::updatable() {
    return mDataUpdatable;
}

void VideoDataManager::requestTimelineData() {
    if (ready() && (unprepared() || updatable())) {
        mDataReady = false;
        mDataUpdatable = false;
        VideoSourceBLL::instance()->searchVideoTimeline(this, [this](const Result<Timeline> &timeline) {
            if (!timeline.has_value()) {
                VideoDebug(MyDebug::WARNING) << "timeline get failed!";
                return;
            }
            mDataReady = true;
            for (auto item : timeline.value()) {
                Q_EMIT timelineDataReply(item->episodesList(), item->dayOfWeek());
            }
            QTimer::singleShot(10000, this, [this]() {
                mDataUpdatable = true;
            });
        });
    }
}

VideoDataManager::VideoDataManager(QObject *parent) : QObject(parent) {
    mDataReady = false;
    mDataUpdatable = true;
}