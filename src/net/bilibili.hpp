#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "client.hpp"
#include "../danmaku.hpp"

class BiliVideoInfo final {
    public:
        QString bvid;
        QString aid;
};
class BiliVideoSource final {
    public:
        QStringList urls;
        QByteArray referHeader; //< Header of refer
};
class BiliUrlParse final {
    public:
        QString bvid;      //< BVXXX
        QString seasonID;  //< ID of season, juat number, like 1234
        QString episodeID; //< ID of episode, juat number, like 1234
};
class BiliEpisode final {
    public:
        QString id; //< ID of episode, juat number, like 1234
        QString bvid; //< BVID 
        QString cid; //< Cid of episode, juat number, like 1234
        qreal   duration = 0;

        QString cover;

        QString subtitle;
        QString title;
        QString longTitle;
};
class BiliBangumi final {
    public:
        QString seasonID; //< ID of season, juat number, like 1234
        QString cover; //< Url of cover
        QString title;
        QString alias; //< Alias name
        QString jpTitle; //< Title in japanese format
        QString orgTitle; //< Org title
        QString evaluate; //< Evaluate

        QList<BiliEpisode> episodes;
};
class BiliTimelineEpisode final {
    public:
        QString   pubTime;
        QString   pubIndex; //< Index name
        QString   title;

        QString   cover;
        QString   epCover;
        QString   squareCover;

        QString   episodeID; //< ID of episode, juat number, like 1234
};
class BiliTimelineDay final {
    public:
        QDateTime date;
        int       dayOfWeek;
        QList<BiliTimelineEpisode> episodes;
};

using BiliBangumiList = QList<BiliBangumi>;
using BiliTimeline    = QList<BiliTimelineDay>;   

class BiliClient final : public QObject {
    Q_OBJECT
    public:
        BiliClient(QObject *parent = nullptr);
        ~BiliClient();

        /**
         * @brief fetch danmaku by cid
         * 
         * @param cid 
         * @return The async result of this request
         */
        NetResult<DanmakuList> fetchDanmaku(const QString &cid);
        /**
         * @brief fetch danmaku xml file
         * 
         * @param cid 
         * @return NetResult<DanmakuList> 
         */
        NetResult<QByteArray> fetchDanmakuXml(const QString &cid);
        /**
         * @brief fetch danmaku protobuf
         * 
         * @param cid 
         * @return NetResult<QByteArray> 
         */
        NetResult<DanmakuList> fetchDanmakuProtobuf(const QString &cid, int segment);
        /**
         * @brief Get suggestions
         * 
         * @param text 
         * @return NetResult<QStringList> 
         */
        NetResult<QStringList> fetchSearchSuggestions(const QString &text);
        /**
         * @brief fetch file by
         * 
         * @param url 
         * @return NetResult<QByteArray> 
         */
        NetResult<QByteArray>  fetchFile(const QString &url);
        /**
         * @brief Get bangumi info by episode ID
         * 
         * @param episodeID 
         * @return NetResult<BiliBangumi> 
         */
        NetResult<BiliBangumi> fetchBangumiByEpisodeID(const QString &episodeID);
        /**
         * @brief Get bangumi info by season ID
         * 
         * @param seasonID 
         * @return NetResult<BiliBangumi> 
         */
        NetResult<BiliBangumi> fetchBangumiBySeasonID(const QString &seasonID);
        /**
         * @brief fetch danmaku video info
         * 
         * @param bvid The bvid of the video
         * @return The async result of this request
         */
        NetResult<BiliVideoInfo> fetchVideoInfo(const QString &bvid);
        /**
         * @brief Get video source
         * 
         * @param cid
         * @param bvid 
         * @return NetResult<BiliVideoSource> 
         */
        NetResult<BiliVideoSource> fetchVideoSource(const QString &cid, const QString &bvid);
        /**
         * @brief Seaech for bangumi
         * 
         * @param what 
         * @return NetResult<BiliBangumiList> 
         */
        NetResult<BiliBangumiList> searchBangumi(const QString &what);
        /**
         * @brief Get the timeline data for the bangumi
         * 
         * @param kind
         * @param before 
         * @param after 
         * @return NetResult<BiliTimeline> 
         */
        NetResult<BiliTimeline>    fetchTimeline(int kind = 1, int before = 7, int after = 7);
        /**
         * @brief Convert bvid or avid to cid
         * 
         * @param bvid 
         * @return The async result of this request 
         */
        NetResult<QString> convertToCid(const QString &bvid);
    public:
        /**
         * @brief Get BVID from URL
         * 
         * @param url The url of it
         * @return Result<QString> 
         */
        Result<QString> parseBvid(const QString &url);
        /**
         * @brief Parse url from bilibili, get bvid ssid epid from the url
         * 
         * @param url 
         * @return BiliUrlParse 
         */
        BiliUrlParse    parseUrl(const QString &url);

        static QString  avidToBvid(uint64_t avid);
        static uint64_t bvidToAvid(const QString &bvid);
    private:
        NetResult<BiliBangumi> fetchBangumiInternal(const QString &seasonID, const QString &episodeID);
        void                   fetchCookie();

        QNetworkAccessManager manager; //< Manager to access it
        bool                  hasCookie = false; //< Try Get the cookie
};

inline NetResult<BiliBangumi> BiliClient::fetchBangumiByEpisodeID(const QString &episodeID) {
    return fetchBangumiInternal(QString(), episodeID);
}
inline NetResult<BiliBangumi> BiliClient::fetchBangumiBySeasonID(const QString &seasonID) {
    return fetchBangumiInternal(seasonID, QString());
}