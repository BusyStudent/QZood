#include "../log.hpp"
#include "bilibili.hpp"
#include <QNetworkCookieJar>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QImage>

#if !defined(QZOOD_NO_PROTOBUF)
#include "dm.pb.h"
#endif

BiliClient::BiliClient(QObject *parent) : QObject(parent) {
    manager.setCookieJar(new QNetworkCookieJar(&manager));

}
BiliClient::~BiliClient() {

}
NetResult<DanmakuList> BiliClient::fetchDanmaku(const QString &cid) {
#if !defined(QZOOD_NO_PROTOBUF)
    class Routinue : public std::enable_shared_from_this<Routinue> {
        public:
            ~Routinue() {

            }
            QString cid; //< CID
            int     idx = 1; //< Current Index
            DanmakuList dans;
            NetResult<DanmakuList> result;
            BiliClient *client;

            void process(const Result<DanmakuList> &dan) {
                if (!dan) {
                    // Eof
                    if (dans.empty()) {
                        result.putResult(std::nullopt);
                    }
                    else {
                        result.putResult(dans);
                    }
                    return;
                }
                // Merge
                dans = MergeDanmaku(dans, dan.value());
                idx += 1;

                client->fetchDanmakuProtobuf(cid, idx).
                    then([self = this->shared_from_this()](const Result<DanmakuList> &d) mutable {
                        self->process(d);
                });
            }
    };
    auto result   = NetResult<DanmakuList>::Alloc();
    auto routinue = std::make_shared<Routinue>();
    routinue->cid = cid;
    routinue->idx = 1;
    routinue->result = result;
    routinue->client = this;

    fetchDanmakuProtobuf(cid, 1).then([routinue](const Result<DanmakuList> &d) {
        routinue->process(d);
    });

    return result;
#else
    auto result = NetResult<DanmakuList>::Alloc();
    fetchDanmakuXml(cid).then([result](const Result<QByteArray> &xml) mutable {
        Result<DanmakuList> danmaku;
        if (xml) {
            danmaku = ParseDanmaku(xml.value());
        }

        result.putResult(danmaku);
    });
    return result;
#endif
}

#if !defined(QZOOD_NO_PROTOBUF)
NetResult<DanmakuList> BiliClient::fetchDanmakuProtobuf(const QString &cid, int segment) {
    auto result = NetResult<DanmakuList>::Alloc();
    fetchFile(QString("https://api.bilibili.com/x/v2/dm/web/seg.so?oid=%1&type=1&segment_index=%2").arg(cid, QString::number(segment)))
        .then(this, [result](const Result<QByteArray> &binary) mutable {
            Result<DanmakuList> danmaku;
            if (binary) {
                ::bilibili::community::service::dm::v1::DmSegMobileReply reply;

                if (reply.ParseFromArray(binary.value().data(), binary.value().size())) {
                    DanmakuList list;

                    for (int idx = 0; idx < reply.elems_size(); idx++) {
                        auto &elem = reply.elems(idx);

                        DanmakuItem item;
                        item.position = qreal(elem.progress()) / 1000.0; //< To S
                        item.type     = DanmakuItem::Type(elem.mode());
                        item.size     = DanmakuItem::Size(elem.fontsize());

                        item.color    = QColor(elem.color());
                        item.text     = QString::fromUtf8(elem.content());
                        item.pool     = DanmakuItem::Pool(elem.pool());
                        item.level    = elem.weight();

                        list.push_back(item);
                    }

                    SortDanmaku(&list);
                    danmaku = list;
                }
                else {
                    QJsonParseError error;
                    auto json = QJsonDocument::fromJson(binary.value(), &error);

                    if (!json.isNull()) {
                        qDebug() << "BiliClient::fetchDanmakuProtobuf error" << json;
                    }
                }
            }
            result.putResult(danmaku);
        })
    ;
    return result;
}
#endif
NetResult<QStringList> BiliClient::fetchSearchSuggestions(const QString &text) {
    auto result = NetResult<QStringList>::Alloc();
    if (text.isEmpty()) {
        result.putLater(std::nullopt);
        return result;
    }

    fetchFile(QString("https://s.search.bilibili.com/main/suggest?term=%1&main_ver=v1").arg(text)).then(
        this, [result](const Result<QByteArray> &jsondata) mutable {

        Result<QStringList> list;
        if (jsondata) {
            QJsonParseError error;
            auto json = QJsonDocument::fromJson(jsondata.value(), &error);

            // Debug print
            // qDebug() << "fetchSearchSuggestions reply" << json;
            if (!json.isNull() && json["code"] == 0) {
                QStringList v;
                for (auto item : json["result"]["tag"].toArray()) {
                    v.push_back(item.toObject()["value"].toString());
                }
                list = v;
            }
        }

        result.putResult(list);
    });


    return result;
}
NetResult<QByteArray> BiliClient::fetchDanmakuXml(const QString &cid) {
    return fetchFile(QString("https://api.bilibili.com/x/v1/dm/list.so?oid=%1").arg(cid));
}
NetResult<QByteArray> BiliClient::fetchFile(const QString &url) {
    QNetworkRequest request;

    // Mark request
    request.setUrl(url);
    request.setRawHeader("User-Agent", RandomUserAgent());

    auto result = NetResult<QByteArray>::Alloc();
    auto reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, result]() mutable {
        qDebug() << "BiliClient::fetchFile " 
                 << reply->url().toString() 
                 << " Status code: " 
                 << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
        ;

        Result<QByteArray> data;

        reply->deleteLater();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
            data = reply->readAll();
        }
        else {
            qDebug() << "Failed to fetch " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        }

        result.putResult(data);
    });

    return result;
}
NetResult<QString> BiliClient::convertToCid(const QString &bvid) {
    QNetworkRequest request;
    QString url = "https://api.bilibili.com/x/player/pagelist";

    url += QString("?bvid=%1").arg(bvid);

    // qDebug() << "Prepare for " << url;

    request.setUrl(url);
    request.setRawHeader("User-Agent", RandomUserAgent());

    auto result = NetResult<QString>::Alloc();
    auto reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, result]() mutable {
        Result<QString> cid;

        reply->deleteLater();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(reply->readAll(), &error);

            // qDebug() << doc;

            if (!doc.isEmpty()) {
                if (doc["code"] == 0) {
                    cid = QString::number(doc["data"][0]["cid"].toInt());
                }
                else {
                    // Error
                    ZOOD_QLOG("Failed to fetch message %1", doc["message"].toString());
                    qDebug() << doc;
                }
            }
        }

        result.putResult(cid);
    });

    return result;
}
NetResult<BiliVideoSource> BiliClient::fetchVideoSource(const QString &cid, const QString &bvid) {
    QNetworkRequest request;
    QString url = QString("https://api.bilibili.com/x/player/playurl?qn=64&cid=%1&bvid=%2").arg(cid, bvid);

    
    // qDebug() << "Prepare for " << url;

    request.setUrl(url);
    request.setRawHeader("User-Agent", RandomUserAgent());

    auto result = NetResult<BiliVideoSource>::Alloc();
    auto reply = manager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, result]() mutable {
        // Process response
        Result<BiliVideoSource> source;

        reply->deleteLater();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(reply->readAll(), &error);

            // qDebug() << doc;

            if (!doc.isEmpty() && doc["code"] == 0) {
                BiliVideoSource bilisource;
                bilisource.referHeader = "https://www.bilibili.com";
                for (const auto &item : doc["data"]["durl"].toArray()) {
                    bilisource.urls.push_back(item.toObject()["url"].toString());
                }
                source = std::move(bilisource);
            }
        }

        result.putResult(source);
    });

    return result;
}
NetResult<BiliBangumiList> BiliClient::searchBangumi(const QString &name) {
    auto result = NetResult<BiliBangumiList>::Alloc();
    // Todo cookie here
    if (!hasCookie) {
        fetchFile("https://www.bilibili.com").then([result, name, this](const Result<QByteArray> &b) mutable {
            hasCookie = true;
            searchBangumi(name).then([result](const Result<BiliBangumiList> &r) mutable {
                result.putResult(r);
            });
        });
        return result;
    }

    fetchFile(QString("https://api.bilibili.com/x/web-interface/search/type?search_type=media_bangumi&keyword=%1").arg(name)).then(this, [result](const Result<QByteArray> &data) mutable {
        Result<BiliBangumiList> list;
        if (!data) {
            result.putResult(list);
            return;
        }
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(data.value(), &error);

        if (doc.isEmpty() || !(doc["code"] == 0)) {
            qDebug() << doc;
            result.putResult(list);
            return;
        }

        BiliBangumiList blist;

        for (auto jitem : doc["data"]["result"].toArray()) {
            auto item = jitem.toObject();

            BiliBangumi ban;
            ban.seasonID = QString::number(item["season_id"].toInt());
            ban.cover = item["cover"].toString();
            ban.title = item["title"].toString();
            ban.orgTitle = item["org_title"].toString();
            ban.evaluate = item["desc"].toString();

            // Clean the title
            ban.title.replace("\u003cem class=\"keyword\"\u003e", "").replace("\u003c/em\u003e", "");
            ban.orgTitle.replace("\u003cem class=\"keyword\"\u003e", "").replace("\u003c/em\u003e", "");
            
            // For 
            for (auto jitem : item["eps"].toArray()) {
                auto ep = jitem.toObject();

                BiliEpisode episode;
                episode.id = QString::number(ep["id"].toInt());
                episode.cover = ep["cover"].toString();
                episode.title = ep["title"].toString();
                episode.longTitle = ep["long_title"].toString();

                ban.episodes.push_back(episode);
            }

            blist.push_back(ban);
        }

        list = blist;
        result.putResult(list);
    });

    return result;
}
NetResult<BiliBangumi> BiliClient::fetchBangumiInternal(const QString &seasonID, const QString &episodeID) {
    QString url = QString("https://api.bilibili.com/pgc/view/web/season?");
    if (!seasonID.isEmpty()) {
        url += "season_id=";
        url += seasonID;
    }
    else {
        url += "ep_id=";
        url += episodeID;
    }
    
    auto result = NetResult<BiliBangumi>::Alloc();

    fetchFile(url).then([result](const Result<QByteArray> &data) mutable {
        Result<BiliBangumi> ban;
        if (data) {
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(data.value(), &error);
            if (doc["code"] == 0) {
                BiliBangumi bangumi;

                auto result = doc["result"];

                bangumi.alias = result["alias"].toString();
                bangumi.cover = result["cover"].toString();
                bangumi.evaluate = result["evaluate"].toString();
                bangumi.title = result["title"].toString();
                bangumi.jpTitle  = result["jp_title"].toString();

                for (auto elem : result["episodes"].toArray()) {
                    BiliEpisode eps;
                    auto object = elem.toObject();

                    eps.bvid = object["bvid"].toString();
                    eps.cid = QString::number(object["cid"].toInteger());
                    eps.id = QString::number(object["id"].toInteger());
                    eps.duration = object["duration"].toInteger();

                    eps.cover = object["cover"].toString();

                    eps.longTitle = object["long_title"].toString();
                    eps.title = object["title"].toString();
                    eps.subtitle = object["subtitle"].toString();

                    bangumi.episodes.push_back(eps);
                }

                ban = bangumi;
            }
        }
        result.putResult(ban);
    });

    return result;
}
NetResult<BiliTimeline> BiliClient::fetchTimeline(int kind, int before, int after) {
    auto url = QString("https://api.bilibili.com/pgc/web/timeline?types=%1&before=%2&after=%3").arg(
        QString::number(kind), 
        QString::number(before), 
        QString::number(after)
    );
    auto result = NetResult<BiliTimeline>::Alloc();
    fetchFile(url).then([result](const Result<QByteArray> &data) mutable {
        if (!data) {
            result.putResult(std::nullopt);
            return;
        }
        auto json = QJsonDocument::fromJson(data.value());
        if (json.isNull()) {
            result.putResult(std::nullopt);
            return;
        }

        qDebug() << json;

        // Begin Parse
        if (json["code"] != 0) {
            qDebug() << json["message"];

            result.putResult(std::nullopt);
            return;
        }
        BiliTimeline timeline;
        auto years = QDateTime::currentDateTime().date().year();

        for (auto _day : json["result"].toArray()) {
            BiliTimelineDay day;
            auto data = _day.toObject();

            day.dayOfWeek = data["day_of_week"].toInt();

            auto dateString = data["date"].toString();
            // Like 2-3
            int month;
            int dayt;
            sscanf(dateString.toUtf8().constData(), "%d-%d", &month, &dayt);

            day.date = QDate(years, month, dayt);

            for (auto _ep : data["episodes"].toArray()) {
                BiliTimelineEpisode ep;
                auto data = _ep.toObject();

                ep.pubTime = data["pub_time"].toString();
                ep.pubIndex = data["pub_index"].toString();

                ep.epCover = data["ep_cover"].toString();
                ep.squareCover = data["square_cover"].toString();
                ep.cover = data["cover"].toString();

                ep.title = data["title"].toString();

                ep.episodeID = QString::number(data["episode_id"].toInteger());

                day.episodes.push_back(ep);
            }

            timeline.push_back(day);
        }

        result.putResult(timeline);
    });

    return result;
}
void BiliClient::fetchCookie() {
    // TODO Get cookie here
    fetchFile("https://www.bilibili.com").then(this, [this](const Result<QByteArray> &) {
        hasCookie = true;
    });
}

// Parse Utils
Result<QString> BiliClient::parseBvid(const QString &url) {
    int bvbegin = url.indexOf("BV");
    if (bvbegin < 0) {
        return std::nullopt;
    }
    int bvend = url.indexOf("/", bvbegin);
    if (bvend < 0) {
        return std::nullopt;
    }
    return url.mid(bvbegin, bvend - bvbegin);
}
BiliUrlParse    BiliClient::parseUrl(const QString &url) {
    BiliUrlParse result;
    
    // BVID here
    int bvbegin = url.indexOf("BV");
    if (bvbegin >= 0) {
        int bvend = url.indexOf("/", bvbegin);
        if (bvend >= 0) {
            result.bvid = url.mid(bvbegin, bvend - bvbegin);
        }
    }
    if (url.contains("bangumi")) {
        // Url like this 
        // https://www.bilibili.com/bangumi/play/ep732395?sxxx
        // Season ID here
        int ssbegin = url.indexOf("ss");
        if (ssbegin >= 0) {
            QString seasonID;
            int ssend = url.indexOf("?", ssend);

            // Skip SS prefix
            if (ssend >= 0) {
                seasonID = url.mid(ssbegin + 2, ssend - ssbegin - 2);
            }
            else {
                seasonID = url.mid(ssbegin + 2);
            }

            // Check can onvert to number
            bool ok;
            seasonID.toInt(&ok);
            if (ok) {
                result.seasonID = seasonID;
            }
        }

        // Episode id here, as same as Season ID
        int epbegin = url.indexOf("ep");
        if (epbegin >= 0) {
            QString episodeID;
            int epend = url.indexOf("?", epbegin);

            if (epend >= 0) {
                episodeID = url.mid(epbegin + 2, epend - epbegin - 2);
            }
            else {
                episodeID = url.mid(epbegin + 2);
            }

            bool ok;
            episodeID.toInt(&ok);
            if (ok) {
                result.episodeID = episodeID;
            }
        }
    }

    return result;
}

// Convert Utils

// From https://github.com/SocialSisterYi/bilibili-API-collect/blob/master/docs/other/bvid_desc.md

static constexpr char table[] = "fZodR9XQDSUm21yCkr6zBqiveYah8bt4xsWpHnJE7jL5VG3guMTKNPAwcF"; // 码表
static constexpr uint64_t XOR = 177451812; // 固定异或值
static constexpr uint64_t ADD = 8728348608; // 固定加法值
static constexpr int s[] = {11, 10, 3, 8, 4, 6}; // 位置编码表
// static char trTable[124]; // 反查码表
static constexpr char trTable [74] = {
    13, 12, 46, 31, 43, 18, 40, 28,  5,  0,  0,  0,  0,  0,  0,  0, 54, 20, 15, 8,
    39, 57, 45, 36,  0, 38, 51, 42, 49, 52,  0, 53,  7,  4,  9, 50, 10, 44, 34, 6,
    25,  1,  0,  0,  0,  0,  0,  0, 26, 29, 56,  3, 24,  0, 47, 27, 22, 41, 16, 0,
    11, 37,  2, 35, 21, 17, 33, 30, 48, 23, 55, 32, 14, 19,
};


// 初始化反查码表
static void tr_init() {
	// for (int i = 0; i < 58; i++)
	// 	trTable[table[i]] = i;
}

QString  BiliClient::avidToBvid(uint64_t av) {
    tr_init();

	char *result = (char*)::malloc(13);
	::strcpy(result,"BV1  4 1 7  ");
	av = (av ^ ::XOR) + ::ADD;
	for (int i = 0; i < 6; i++)
		result[s[i]] = ::table[(uint64_t)(av / (uint64_t)::pow(58, i)) % 58];
    auto ret = QString::fromLocal8Bit(result);
    ::free(result);
	return ret;
}
uint64_t BiliClient::bvidToAvid(const QString &bvid) {
    tr_init();

    auto u8 = bvid.toUtf8();
    auto bv = u8.constData();

    uint64_t r = 0;
	uint64_t av;
	for (int i = 0; i < 6; i++)
		r += trTable[bv[s[i]]] * (uint64_t)pow(58, i);
	av = (r - ::ADD) ^ ::XOR;
	return av;

}


// BilibiliWrapper
namespace {

constexpr auto BSourceName = "Bilibili";

class BEpisode : public Episode {
public:
    BEpisode(BiliEpisode b, BiliClient &client) : data(b), client(client) { }

    QString title() override {
        return data.title;
    }
    NetResult<QImage> fetchCover() override {
        auto r = NetResult<QImage>::Alloc();
        client.fetchFile(data.cover).then([r](const Result<QByteArray> &b) mutable {
            if (!b) {
                r.putResult(std::nullopt);
                return;
            }
            auto image = QImage::fromData(b.value());
            if (image.isNull()) {
                r.putResult(std::nullopt);
                return;
            }
            r.putResult(image);
        });
        return r;
    }
    QStringList sourcesList() override {
        return QStringList(BSourceName);
    }
    QStringList danmakuSourceList() override {
        return QStringList(BSourceName);
    }
    NetResult<QString> fetchVideo(const QString &sourceString) override {
        if (sourceString != BSourceName) {
            return NetResult<QString>::Alloc().putLater(std::nullopt);
        }
        auto r = NetResult<QString>::Alloc();
        client.fetchVideoSource(data.cid, data.bvid).then([r](const Result<BiliVideoSource> &s) mutable {
            if (!s) {
                r.putResult(std::nullopt);
                return;
            }
            r.putResult(s.value().urls.first());
        });
        return r;
    }
    NetResult<DanmakuList> fetchDanmaku(const QString &sourceString) {
        if (sourceString != BSourceName) {
            return NetResult<DanmakuList>::Alloc().putLater(std::nullopt);
        }
        return client.fetchDanmaku(data.cid);
    }
private:
    BiliEpisode data;
    BiliClient &client;
};

class BBangumi : public Bangumi {
public:
    BBangumi(BiliBangumi b, BiliClient &client) : data(b), client(client) { }

    NetResult<EpisodeList> fetchEpisodes() override {
        EpisodeList list;
        for (const auto &each : data.episodes) {
            list.push_back(std::make_shared<BEpisode>(each, client));
        }

        return NetResult<EpisodeList>::Alloc().putLater(list);
    }
    NetResult<QImage> fetchCover() override {
        auto r = NetResult<QImage>::Alloc();
        client.fetchFile(data.cover).then([r](const Result<QByteArray> &b) mutable {
            if (!b) {
                r.putResult(std::nullopt);
                return;
            }
            auto image = QImage::fromData(b.value());
            if (image.isNull()) {
                r.putResult(std::nullopt);
                return;
            }
            r.putResult(image);
        });
        return r;
    }
    QStringList availableSource() override {
        return QStringList(BSourceName);
    }
    QString description() override {
        return data.evaluate;
    }
    QString title() override {
        return data.title;
    }
private:
    BiliBangumi data;
    BiliClient &client;
};
class BTimelineItem : public TimelineItem, public QObject {
public:
    BTimelineItem(BiliTimelineDay b, BiliClient &client) : data(b), client(client) { }

    QDate date() override {
        return data.date;
    }
    int       dayOfWeek() override {
        return data.dayOfWeek;
    }
    NetResult<BangumiList> fetchBangumiList() override {
        if (requestLeft > 0) {
            // Has current request
            return pendingPromise;
        }
        if (requestLeft == 0) {
            // Already fetched
            return NetResult<BangumiList>::Alloc().putLater(outList);
        }

        auto r = NetResult<BangumiList>::Alloc();
        pendingPromise = r;
        requestLeft = data.episodes.size();

        for (const auto &ep : data.episodes) {
            client.fetchBangumiByEpisodeID(ep.episodeID)
                .then(this, [this, r](const Result<BiliBangumi> &ban) mutable {
                    if (ban) {
                        outList.push_back(std::make_shared<BBangumi>(ban.value(), client));
                    }
                    requestLeft -= 1;

                    if (requestLeft == 0) {
                        // Done
                        r.putResult(outList);

                        // Unref the value we goted
                        pendingPromise = NetResult<BangumiList>();
                    }
            });
        }
        return r;
    }
private:
    BiliTimelineDay data;
    BiliClient &client;
    int requestLeft = -1;
    BangumiList outList;

    NetResult<BangumiList> pendingPromise;
};

class BClient : public VideoInterface {
public:
    QString name() override {
        return BSourceName;
    }
    NetResult<BangumiList> searchBangumi(const QString &text) override {
        auto r = NetResult<BangumiList>::Alloc();
        client.searchBangumi(text).then(this, [r, this](const Result<BiliBangumiList> &bans) mutable {
            if (!bans) {
                r.putResult(std::nullopt);
                return;
            }
            BangumiList outList;
            for (const auto &each : bans.value()) {
                outList.push_back(std::make_shared<BBangumi>(each, client));
            }
            r.putResult(outList);
        });
        return r;
    }
    NetResult<Timeline> fetchTimeline() override {
        auto r = NetResult<Timeline>::Alloc();
        client.fetchTimeline().then(this, [r, this](const Result<BiliTimeline> &t) mutable{
            if (!t) {
                r.putResult(std::nullopt);
                return;
            }
            Timeline timeline;
            for (const auto &each : t.value()) {
                timeline.push_back(std::make_shared<BTimelineItem>(each, client));
            }
            r.putResult(timeline);
        });
        return r;
    }
private:
    BiliClient client;
};

}


ZOOD_REGISTER_VIDEO_INTERFACE(BClient);