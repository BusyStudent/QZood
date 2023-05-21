#include "../log.hpp"
#include "bilibili.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#if !defined(QZOOD_NO_PROTOBUF)
#include "dm.pb.h"
#endif

BiliClient::BiliClient(QObject *parent) : QObject(parent) {

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
            qDebug() << "fetchSearchSuggestions reply" << json;
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
        Result<QByteArray> data;

        reply->deleteLater();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
            data = reply->readAll();
        }

        result.putResult(data);
    });

    return result;
}
NetResult<QString> BiliClient::convertToCid(const QString &bvid) {
    QNetworkRequest request;
    QString url = "https://api.bilibili.com/x/player/pagelist";

    url += QString("?bvid=%1").arg(bvid);

    qDebug() << "Prepare for " << url;

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

            qDebug() << doc;

            if (!doc.isEmpty()) {
                if (doc["code"] == 0) {
                    cid = QString::number(doc["data"][0]["cid"].toInt());
                }
                else {
                    // Error
                    ZOOD_QLOG("Failed to fetch message %1", doc["message"].toString());
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

    
    qDebug() << "Prepare for " << url;

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

            qDebug() << doc;

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
