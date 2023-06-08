#include "promise.hpp"
#include <QDateTime>

#include "../danmaku.hpp"

class Episode;
class Bangumi;
class TimelineItem;

using EpisodePtr = RefPtr<Episode>;
using BangumiPtr = RefPtr<Bangumi>;
using EpisodeList = QList<EpisodePtr>;
using BangumiList = QList<BangumiPtr>;
using TimelineItemPtr = RefPtr<TimelineItem>;


class Episode : public DynRefable {
    public:
        /**
         * @brief Get title of the episode
         * 
         * @return QString 
         */
        virtual QString title() = 0;
        /**
         * @brief Get the icon of the episode
         * 
         * @return NetResult<QImage> 
         */
        virtual NetResult<QImage> icon() = 0;

        /**
         * @brief Get the avliable source of the episode
         * 
         * @return QStringList 
         */
        virtual QStringList sourcesList() = 0;
        /**
         * @brief Get the recommended source of the episode
         * 
         * @return QString 
         */
        virtual QString     recommendedSource() = 0;
        /**
         * @brief Get details video url of sourceString
         * 
         * @param sourceString 
         * @return NetResult<QString> 
         */
        virtual NetResult<QString> fetchVideo(const QString &sourceString) = 0;


        virtual QStringList danmakuSourceList() = 0;
        virtual NetResult<DanmakuList> fetchDanmaku(const QString &what) = 0;


};

class Bangumi : public DynRefable {
    public:
        /**
         * @brief Get the list of available source (like bilibili or 樱花动漫)
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

class TimelineItem : public DynRefable {
    public:
        /**
         * @brief Get the date of the timelineItem
         * 
         * @return QDateTime 
         */
        virtual QDateTime date() = 0;
        /**
         * @brief Get the time of day of week
         * 
         * @return int 
         */
        virtual int       dayOfWeek() = 0;
        /**
         * @brief Get the list of bangumi that days
         * 
         * @return QList<RefPtr<Bangumi>> 
         */
        virtual NetResult<BangumiList> fetchBangumiList() = 0;
};

/**
 * @brief Interface for Timeline
 * 
 */
class Timeline : public DynRefable {
    public:
        using ItemPtr = RefPtr<TimelineItem>;

        /**
         * @brief Get each day items
         * 
         * @return QList<RefPtr<Item>> 
         */
        virtual QList<ItemPtr> items() = 0;
};

/**
 * @brief Pure virtual interface for access the data structure
 * 
 */
class DataService {
    public:
        // virtual NetPromise<QStringList> fetchData() = 0;
        virtual NetResultPtr<Timeline> fetchTimeline() = 0;
        // virtual NetResult<BangumiList> search(const QString &) = 0;

        static DataService *instance();
    protected:
        DataService() = default;
        ~DataService() = default;
};
