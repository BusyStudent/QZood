#include "videoSourceBLL.hpp"

#include "../../net/client.hpp"
#include "../../net/bilibili.hpp"

class VideoSourceBLLHelper {
public:
    VideoSourceBLLHelper() {
        dataSevrice = DataService::instance();
    }
    template<typename FuncT, typename ...Arg>
    void doFunc(QList<FuncT>& queue, Arg&& ...arg) {
        for (const auto& func : queue) {
            func(std::forward<Arg>(arg)...);
        }
        queue.clear();
    }
    void requestTimeline(QObject *obj, std::function<void(const Result<Timeline>&)> func) {
        timelineQueue.push_back(func);
        if (timelineQueue.size() > 1) {
            return;
        }
        dataSevrice->fetchTimeline().then(obj, [this](const Result<Timeline>& timeline) {
            if (timeline.has_value()) {
                doFunc(timelineQueue, timeline.value());
            }
            doFunc(timelineQueue, Result<Timeline>());
        });
    }

public:
    DataService* dataSevrice;

private:
    QList<std::function<void(const Result<Timeline>&)> > timelineQueue;
};

VideoSourceBLL::VideoSourceBLL(QObject *parent) : QObject(parent), d(new VideoSourceBLLHelper()), client(new BiliClient()) {}

VideoSourceBLL::~VideoSourceBLL() { }

void VideoSourceBLL::searchSuggestion(const QString& text, QObject *obj, std::function<void(const Result<QStringList>&)> func) {
    // std::shared_ptr<int> counter = std::make_shared<int>(videoSources.size());
    if (obj == nullptr) {
        obj = this;
    }
    client->fetchSearchSuggestions(text).then(obj, func);
}

void VideoSourceBLL::searchVideoTimeline(QObject *obj, std::function<void(const Result<Timeline>&)> func) {
    if (obj == nullptr) {
        obj = this;
    }
    if (dirty & TIMELINE) {
        d->requestTimeline(obj, [this, func](const Result<Timeline>& timeline) {
            dirty ^= TIMELINE;
            videoTimelineItems.clear();
            bangumis.clear();
            videos.clear();
            videoTimelineItems = timeline.value();
            func(videoTimelineItems);
        });
    }
}

Timeline VideoSourceBLL::videoInWeek(TimeWeek week) {
    Timeline result;
    for (auto item : videoTimelineItems) {
        if (item->dayOfWeek() == int(week)) {
            result.push_back(item);
        }
    }
    return result;
}

void VideoSourceBLL::searchBangumiFromTimeline(Timeline t,QObject *obj, std::function<void(const Result<BangumiList>&)> func) {
    if (obj == nullptr) {
        obj = this;
    }
    if (t.isEmpty()) {
        func(Result<BangumiList>());
        return ;
    }
    if (bangumis.contains((TimeWeek)(t[0]->dayOfWeek()))) {
        // 确认当前番剧列表存在对应数据时直接回调
        func(bangumis[(TimeWeek)(t[0]->dayOfWeek())]);
    } else {
        bangumis[(TimeWeek)(t[0]->dayOfWeek())].clear();
        // 当前缓存中不存在数据，调用数据接口fetch数据
        std::shared_ptr<int> counter = std::make_shared<int>(t.size());
        for (auto& timelineItem : t) {
            timelineItem->fetchBangumiList().then([this, counter, func, week = (TimeWeek)(timelineItem->dayOfWeek())](const Result<BangumiList>& bangumiList){
                if(bangumiList.has_value()) {
                    bangumis[week].append(bangumiList.value());
                }
                (*counter)--;
                if (0 == (*counter)) {
                    func(bangumis[week]);
                }
            });
        }
    }
}

void VideoSourceBLL::searchVideosFromBangumi(BangumiPtr b, QObject *obj, std::function<void(const Result<VideoBLLList>&)> func) {
    if (obj == nullptr) {
        obj = this;
    }
    if (videos.contains(mapToTitle(b)) && videos[mapToTitle(b)].size() > 0) {
        func(videos[mapToTitle(b)]);
    } else {
        b->fetchEpisodes().then([this, func, title = mapToTitle(b)](const Result<EpisodeList>& episodes){
            if(episodes.has_value()) {
                videos[title].clear();
                for (auto& episode : episodes.value()) {
                    videos[title].push_back(createVideoBLL(EpisodeList{episode}));
                }
                func(videos[title]);
            } else {
                func(Result<VideoBLLList>());
            }
        });
    }
}

void VideoSourceBLL::searchBangumiFromText(const QString& text, QObject *obj, std::function<void(const Result<BangumiList>&)> func) {
    if (obj == nullptr) {
        obj = this;
    }
    d->dataSevrice->searchBangumi(text).then(obj, func);
}

void VideoSourceBLL::searchVideosFromTitle(const QString& title, QObject *obj, std::function<void(const Result<VideoBLLList>&)> func) {
    if (obj == nullptr) {
        obj = this;
    }
    if (videos.contains(title)) {
        qDebug() << "use videos from cache videos map";
        func(videos[title]);
        return ;
    }
    searchBangumiFromText(title, obj, [this, func, title, obj](const Result<BangumiList>& bangumis) {
        if (!bangumis.has_value() || bangumis->size() == 0) {
            qDebug() << "bangumis for " << title << " are not find";
            func(Result<VideoBLLList>());
            return ;
        }
        qDebug() << "video size : " << videos[title].size();
        qDebug() << "bangumiList size : " << bangumis->size();
        searchVideosFromBangumi(bangumis->back(), obj, [this, title, func](const Result<VideoBLLList>& bvideos) {
            videos[title].clear();
            if (bvideos.has_value()){
                videos[title] = bvideos.value();
            }
            if (videos.contains(title)) {
                qDebug() << "search videos size : " << videos[title].size();
                func(videos[title]);
            } else {
                qDebug() << "videos for title " << title << " are not find";
                func(Result<VideoBLLList>());
            }
        });
    });
}

QString VideoSourceBLL::mapToTitle(BangumiPtr b) {
    return b->title();
}

