#pragma once

#include <QList>

#include "../../net/promise.hpp"
#include "videoBLL.hpp"

class VideoInterface;
class BiliClient;

enum TimeWeek : int {
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6,
    SUNDAY = 7,
};

class VideoSourceBLL {
    public:
        enum Update : uint8_t {
            TIMELINE = 1u<<0,
        };

    public:
        static inline VideoSourceBLL& instance() {
            static VideoSourceBLL v;
            return v;
        }
        ~VideoSourceBLL();

        void searchSuggestion(const QString& text, QObject *obj, std::function<void(const Result<QStringList>&)> func);
        void searchVideoTimeline(TimeWeek t,QObject *obj, std::function<void(const Result<Timeline>&)> func);
        void searchBangumiFromTimeline(Timeline t,QObject *obj, std::function<void(const Result<BangumiList>&)> func);
        void searchVideosFromBangumi(BangumiPtr b, QObject *obj, std::function<void(const Result<QList<VideoBLLPtr>>&)> func);
        void searchVideosFromTitle(const QString& title, QObject *obj, std::function<void(const Result<QList<VideoBLLPtr>>&)> func);
        void searchBangumiFromText(const QString& text, QObject *obj, std::function<void(const Result<BangumiList>&)> func);

    private:
        VideoSourceBLL();
        VideoSourceBLL(const VideoSourceBLL&) = delete;
        VideoSourceBLL operator=(const VideoSourceBLL&) = delete;
        Timeline videoInWeek(TimeWeek week);
        QString mapToTitle(BangumiPtr b);

    private:
        DataService* dataSevrice;
        BiliClient *client;
        QList<TimelineItemPtr> videoTimelineItems;
        QMap<TimeWeek, BangumiList> bangumis;
        QMap<QString, QList<VideoBLLPtr>> videos;
        uint8_t dirty = 0xff;
        QObject* placeholder;
};