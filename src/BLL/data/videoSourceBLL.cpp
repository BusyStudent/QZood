#include "videoSourceBLL.hpp"

#include "../../net/client.hpp"
#include "../../net/bilibili.hpp"

VideoSourceBLL::VideoSourceBLL(QObject *parent) : QObject(parent) {
    dataSevrice = DataService::instance();
}

VideoSourceBLL::~VideoSourceBLL() {
}

void VideoSourceBLL::searchSuggestion(const QString& text, QObject *obj, std::function<void(const Result<QStringList>&)> func) {
    // std::shared_ptr<int> counter = std::make_shared<int>(videoSources.size());
    if (obj == nullptr) {
        obj = this;
    }
    if (nullptr == client) {
        client = std::make_shared<BiliClient>();
    }
    client->fetchSearchSuggestions(text).then(obj, func);
}

void VideoSourceBLL::searchVideoTimeline(QObject *obj, std::function<void(const Result<Timeline>&)> func) {
    if (obj == nullptr) {
        obj = this;
    }
    if (dirty & TIMELINE) {
        dirty ^= TIMELINE;
        videoTimelineItems.clear();
        bangumis.clear();
        videos.clear();
        // 获取全部源的时间线。
        dataSevrice->fetchTimeline().then(obj, [this, func](const Result<Timeline>& timeline) {
            if (timeline.has_value()) {
                videoTimelineItems.append(timeline.value());
            }
            qDebug() << "fetch time line";
            func(videoTimelineItems);
        });
    } else {
        func(videoTimelineItems);
    }
}

Timeline VideoSourceBLL::videoInWeek(TimeWeek week) {
    Timeline result;
    for (auto item : videoTimelineItems) {
        qDebug() << item->dayOfWeek();
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
    if (videos.contains(b->title())) {
        func(videos[b->title()]);
    } else {
        b->fetchEpisodes().then([this, func, title = mapToTitle(b)](const Result<EpisodeList>& episodes){
            if(episodes.has_value()) {
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
    dataSevrice->searchBangumi(text).then(obj, func);
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
        if (!bangumis.has_value()) {
            qDebug() << "bangumis for " << title << " are not find";
            func(Result<VideoBLLList>());
            return ;
        }
        std::shared_ptr<std::atomic_int> counter_videos = std::make_shared<std::atomic_int>(bangumis->size());
        for (const auto& bangumi : bangumis.value()) {
            searchVideosFromBangumi(bangumi, obj, [this, counter_videos, title = bangumi->title(), func](const Result<VideoBLLList>& bvideos) {
                if (bvideos.has_value()){
                    videos[title] += bvideos.value();
                }
                (*counter_videos)--;
                if (0 == (*counter_videos)) {
                    if (videos.contains(title)) {
                        qDebug() << "search videos size : " << videos[title].size();
                        func(videos[title]);
                    } else {
                        qDebug() << "videos for title " << title << " are not find";
                        func(Result<VideoBLLList>());
                    }
                }
            });
        }
    });
}

QString VideoSourceBLL::mapToTitle(BangumiPtr b) {
    return b->title();
}

