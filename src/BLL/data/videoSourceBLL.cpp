#include "videoSourceBLL.hpp"

#include "../../net/client.hpp"
#include "../../net/bilibili.hpp"

VideoSourceBLL::VideoSourceBLL() {
    dataSevrice = DataService::instance();
    placeholder = new QObject();
}

VideoSourceBLL::~VideoSourceBLL() {
    delete client;
    delete placeholder;
}

void VideoSourceBLL::searchSuggestion(const QString& text, QObject *obj, std::function<void(const Result<QStringList>&)> func) {
    // std::shared_ptr<int> counter = std::make_shared<int>(videoSources.size());
    if (obj == nullptr) {
        obj = placeholder;
    }
    client = new BiliClient();
    client->fetchSearchSuggestions(text).then(obj, func);
}

void VideoSourceBLL::searchVideoTimeline(TimeWeek t, QObject *obj, std::function<void(const Result<Timeline>&)> func) {
    if (obj == nullptr) {
        obj = placeholder;
    }
    if (dirty & TIMELINE) {
        dirty ^= TIMELINE;
        videoTimelineItems.clear();
        bangumis.clear();
        videos.clear();
        dataSevrice->fetchTimeline().then(obj, [this, func, t](const Result<Timeline>& timeline) {
            if (timeline.has_value()) {
                videoTimelineItems.append(timeline.value());
            }
            func(videoInWeek(t));
        });
    } else {
        func(videoInWeek(t));
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
        obj = placeholder;
    }
    if (t.isEmpty()) {
        func(Result<BangumiList>());
        return ;
    }
    if (!bangumis.contains((TimeWeek)(t[0]->dayOfWeek()))) {
        func(bangumis[(TimeWeek)(t[0]->dayOfWeek())]);
    } else {
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
        obj = placeholder;
    }
    if (!videos.contains(b->title())) {
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
        obj = placeholder;
    }
    dataSevrice->searchBangumi(text).then(obj, func);
}

void VideoSourceBLL::searchVideosFromTitle(const QString& title, QObject *obj, std::function<void(const Result<VideoBLLList>&)> func) {
    if (obj == nullptr) {
        obj = placeholder;
    }
    if (videos.contains(title)) {
        func(videos[title]);
        return ;
    }
    searchBangumiFromText(title, obj, [this, func, obj](const Result<BangumiList>& bangumis) {
        if (!bangumis.has_value()) {
            func(Result<VideoBLLList>());
            return ;
        }
        // 获取所有 bangumis 的videos并将他们缝合到同一个视频数据中。
        std::shared_ptr<std::atomic_int> counter_videos = std::make_shared<std::atomic_int>(bangumis->size());
        for (const auto& bangumi : bangumis.value()) {
            searchVideosFromBangumi(bangumi, obj, [this, counter_videos, title = bangumi->title(), func](const Result<VideoBLLList>& bvideos) {
                if (bvideos.has_value()){
                    videos[title] = bvideos.value();
                }
                (*counter_videos)--;
                if (0 == (*counter_videos)) {
                    if (videos.contains(title)) {
                        func(videos[title]);
                    } else {
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

