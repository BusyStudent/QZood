#include "../log.hpp"
#include "bilibili.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#if !defined(QZOOD_NO_PROTOBUF)
#include "dm.pb.h"
#endif

BiliClient::BiliClient(QObject *parent) : VideoInterface(parent) {

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
        _on_videoCidReplyReady(result, reply);
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
void    BiliClient::_on_videoCidReplyReady(NetResult<QString> &result, QNetworkReply *reply) {
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