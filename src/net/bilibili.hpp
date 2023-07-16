#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "client.hpp"
#include "../danmaku.hpp"

#define BILIBILI_CLIENT_NAME QStringLiteral("Bilibili")

class BiliClient;
class BiliEpisode;
class BiliBangumi;
class BiliTimelineEpisode;
class BiliTimelineItem;

using BiliEpisodeList = QList<RefPtr<BiliEpisode>>;
using BiliBangumiList = QList<RefPtr<BiliBangumi>>;
using BiliTimeline    = QList<RefPtr<BiliTimelineItem>>;   
using BiliTimelineEpisodeList = QList<RefPtr<BiliTimelineEpisode>>;

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
class BiliEpisode final : public Episode {
    public:
        QString title() override;
        QString indexTitle() override;
        QString longTitle() override;
        NetResult<QImage> fetchCover() override;
        QStringList sourcesList() override;
        QString     recommendedSource() override;
        NetResult<QString> fetchVideo(const QString &sourceString) override;
        QStringList danmakuSourceList() override;
        NetResult<DanmakuList> fetchDanmaku(const QString &what) override;
        VideoInterface *rootInterface() override;
    public:
        QString id; //< ID of episode, juat number, like 1234
        QString bvid; //< BVID 
        QString cid; //< Cid of episode, juat number, like 1234
        qreal   duration = 0;

        QString cover;

        QString subtitle;
        QString _title;
        QString _longTitle;

        BiliClient *client = nullptr;
};
class BiliBangumi final : public Bangumi {
    public:
        NetResult<EpisodeList> fetchEpisodes() override;
        VideoInterface *rootInterface() override;
        QStringList availableSource() override;
        QString     description() override;
        QString     title() override;
        NetResult<QImage> fetchCover() override;
    public:
        QString seasonID; //< ID of season, juat number, like 1234
        QString cover; //< Url of cover
        QString alias; //< Alias name
        QString _title;
        QString jpTitle; //< Title in japanese format
        QString orgTitle; //< Org title
        QString evaluate; //< Evaluate

        BiliEpisodeList episodes;

        BiliClient *client = nullptr;
};
class BiliTimelineEpisode final : public TimelineEpisode {
    public:
        QString         bangumiTitle() override;
        QString         pubIndexTitle() override;
        VideoInterface *rootInterface() override;
        bool            hasCover() override;
        QStringList     availableSource() override;
        NetResult<QImage> fetchCover() override;
        NetResult<BangumiPtr> fetchBangumi() override;
    public:
        QString   pubTime;
        QString   pubIndex; //< Index name
        QString   title;

        QString   cover;
        QString   epCover;
        QString   squareCover;

        QString   episodeID; //< ID of episode, juat number, like 1234
        QString   seasonID; //< ID of season, juat number, like 1234

        BiliClient *client = nullptr;
};
class BiliTimelineItem final : public TimelineItem {
    public:
        QDate date() override;
        int   dayOfWeek() override;
        TimelineEpisodeList episodesList() override;
        VideoInterface *rootInterface() override;
    public:
        QDate _date;
        int   _dayOfWeek;
        BiliTimelineEpisodeList episodes;

        BiliClient *client = nullptr;
};


class BiliClient final : public VideoInterface {
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
        NetResult<QStringList> fetchSearchSuggestions(const QString &text) override;
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
        NetResult<BiliBangumiList> searchBangumiInternal(const QString &what);
        /**
         * @brief Get the timeline data for the bangumi
         * 
         * @param kind
         * @param before 
         * @param after 
         * @return NetResult<BiliTimeline> 
         */
        NetResult<BiliTimeline>    fetchTimelineInternal(int kind = 1, int before = 7, int after = 7);
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
    public: //< VideoInterface
        QString                name() override;
        NetResult<BangumiList> searchBangumi(const QString &what) override;
        NetResult<Timeline>    fetchTimeline() override;

        bool                   hasSupport(int s) override;
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