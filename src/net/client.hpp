#pragma once

#include <memory>
#include <QImage>
#include <QList>
#include "promise.hpp"
#include "../common/danmaku.hpp"

class QNetworkReply;

class Episode;
class Bangumi;
class MiniBangumi;
class TimelineItem;
class TimelineEpisode;
class VideoInterface;

using EpisodePtr = RefPtr<Episode>;
using BangumiPtr = RefPtr<Bangumi>;
using EpisodeList = QList<EpisodePtr>;
using BangumiList = QList<BangumiPtr>;
using MiniBangumiPtr = RefPtr<MiniBangumi>;
using MinuBangumiList = QList<MiniBangumiPtr>;
using TimelineItemPtr = RefPtr<TimelineItem>;
using TimelineEpisodePtr = RefPtr<TimelineEpisode>;
using TimelineEpisodeList = QList<TimelineEpisodePtr>;
using Timeline        = QList<TimelineItemPtr>;

class DataObject : public DynRefable {
    public:
        /**
         * @brief Get the name belong to the interface
         * 
         * @return QString 
         */
        virtual VideoInterface *rootInterface() = 0;
};

class Episode : public DataObject {
    public:
        /**
         * @brief Get title of the episode
         * 
         * @return QString 
         */
        virtual QString title() = 0;
        /**
         * @brief Get title for index the episode
         * 
         * @return QString 
         */
        virtual QString indexTitle() = 0;
        /**
         * @brief Get the long title of the episode
         * 
         * @return QString 
         */
        virtual QString longTitle();
        /**
         * @brief Get the cover of the episode
         * 
         * @return NetResult<QImage> 
         */
        virtual NetResult<QImage> fetchCover();

        /**
         * @brief Get the avliable source of the episode
         * 
         * @return QStringList 
         */
        virtual QStringList sourcesList() = 0;
        /**
         * @brief Get the recommended source of the episode, if cannot decode, return empty String
         * 
         * @return QString 
         */
        virtual QString     recommendedSource();
        /**
         * @brief Get details video url of sourceString
         * 
         * @param sourceString 
         * @return NetResult<QString> 
         */
        virtual NetResult<QString> fetchVideo(const QString &sourceString) = 0;

        /**
         * @brief Get Danmaku Source of the episode (default not supported)
         * 
         * @return QStringList 
         */
        virtual QStringList danmakuSourceList();
        /**
         * @brief Get the danmaku by source (default not supported)
         * 
         * @param what 
         * @return NetResult<DanmakuList> 
         */
        virtual NetResult<DanmakuList> fetchDanmaku(const QString &what);
};

class Bangumi : public DataObject {
    public:
        /**
         * @brief fetch episode list information of it
         * 
         * @return NetResult<EpisodeList> 
         */
        virtual NetResult<EpisodeList> fetchEpisodes() = 0;
        /**
         * @brief Get the list of available source (like QList(bilibili, 樱花动漫) or just self)
         * 
         * @return QStringList 
         */
        virtual QStringList availableSource() = 0;
        /**
         * @brief Get description of the bangumi, probably be null string
         * 
         * @return QString 
         */
        virtual QString     description() = 0;
        /**
         * @brief Get the title of the bangumi
         * 
         * @return QString 
         */
        virtual QString     title() = 0;
        /**
         * @brief Get the cover data
         * 
         * @return NetResult<QImage> 
         */
        virtual NetResult<QImage> fetchCover() = 0;
};

class MiniBangumi : public DataObject {
    public:
        /**
         * @brief Get the title of the bangumi
         * 
         * @return QString 
         */
        virtual QString bangumiTitle() = 0;
        /**
         * @brief Check the Episode in timeline proivde the cover
         * 
         * @return true 
         * @return false 
         */
        virtual bool    hasCover() = 0;

        /**
         * @brief Get the available source
         * 
         * @return QStringList 
         */
        virtual QStringList availableSource() = 0;

        /**
         * @brief Try to fetch the episode cover
         * 
         * @return NetResult<QImage> 
         */
        virtual NetResult<QImage> fetchCover() = 0;
        /**
         * @brief Get the bangumi of this episode
         * 
         * @return NetResult<BangumiPtr> 
         */
        virtual NetResult<BangumiPtr> fetchBangumi() = 0;
};

class TimelineEpisode : public MiniBangumi {
    public:
        /**
         * @brief Get the pubTitle of the episode
         * 
         * @return QString 
         */
        virtual QString pubIndexTitle() = 0;
};

class TimelineItem : public DataObject {
    public:
        /**
         * @brief Get the date of the timelineItem
         * 
         * @return QDate 
         */
        virtual QDate date() = 0;
        /**
         * @brief Get the time of day of week
         * 
         * @return int 
         */
        virtual int   dayOfWeek() = 0;
        /**
         * @brief Get the episode of the timeline episode
         * 
         * @return TimelineEpisodeList 
         */
        virtual TimelineEpisodeList episodesList() = 0;
};

/**
 * @brief Interface for searching video sources
 * 
 */
class VideoInterface : public QObject {
    public:
        enum Support {
            TimelineSupport,        // Represents support for timeline functionality
            SearchSupport,          // Represents support for search functionality
            SuggestionsSupport      // Represents support for suggestions functionality
        };
        /**
         * @brief Do search videos
         * 
         * @param video 
         * @return NetResultPtr<BangumiList> 
         */
        virtual NetResult<BangumiList> searchBangumi(const QString& video) = 0;
        /**
         * @brief Get the timeline
         * 
         * @return NetResult<Timeline> 
         */
        virtual NetResult<Timeline>    fetchTimeline()                            ;
        /**
         * @brief Get search suggestions of the text
         * 
         * @param text 
         * @return NetResult<QStringList> 
         */
        virtual NetResult<QStringList> fetchSearchSuggestions(const QString &text);
        virtual QString                name()                              = 0;
        virtual bool                   hasSupport(int sup)                 = 0;
    protected:
        virtual ~VideoInterface() = default;
};

// --- IMPLEMENTATION
inline QString Episode::longTitle() {
    return QString();
}
inline QString Episode::recommendedSource() {
    return QString();
}
inline QStringList Episode::danmakuSourceList() {
    return QStringList();
}

inline NetResult<QImage> Episode::fetchCover() {
    return NetResult<QImage>::Alloc().putLater(std::nullopt);
}

inline NetResult<DanmakuList> Episode::fetchDanmaku(const QString &) {
    return NetResult<DanmakuList>::Alloc().putLater(std::nullopt);
}

inline NetResult<Timeline> VideoInterface::fetchTimeline() {
    return NetResult<Timeline>::Alloc().putLater(std::nullopt);
}
inline NetResult<QStringList> VideoInterface::fetchSearchSuggestions(const QString &) {
    return NetResult<QStringList>::Alloc().putLater(std::nullopt);
}


#define ZOOD_REGISTER_VIDEO_INTERFACE(name)              \
    static bool video__init_##name = []() {              \
        RegisterVideoInterface([]() -> VideoInterface *{ \
            return new name();                           \
        });                                              \
        return true;                                     \
    }();                           

/**
 * @brief Get An random useragent
 * 
 * @return QByteArray 
 */
QByteArray RandomUserAgent();
void       RegisterVideoInterface(VideoInterface *(*fn)());
void       InitializeVideoInterface();
QList<VideoInterface*> &GetVideoInterfaceList();

NetResult<QByteArray>   WrapQNetworkReply(QNetworkReply *reply);
NetResult<QImage>       WrapHttpImagePromise(NetResult<QByteArray> res);

template <typename T, typename U>
inline QList<RefPtr<T> > ToSuperObjectList(const QList<RefPtr<U> > &lt) {
    QList<RefPtr<T> > ret;
    for (const auto &o : lt) {
        ret.push_back(o);
    }
    return ret;
}
template <typename T, typename U>
inline QList<RefPtr<T> > ToDerivedObjectList(const QList<RefPtr<U> > &lt) {
    QList<RefPtr<T> > ret;
    for (const auto &o : lt) {
        ret.push_back(std::dynamic_pointer_cast<T>(o));
    }
    return ret;
}