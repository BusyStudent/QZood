#include "bilibili.hpp"
#include "../log.hpp"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

BiliClient::BiliClient(QObject *parent) : QObject(parent) {

}
BiliClient::~BiliClient() {

}

NetResult<DanmakuList> BiliClient::fetchDanmaku(const QString &cid) {
    QNetworkRequest request;

    // Mark request
    request.setUrl(QString("https://api.bilibili.com/x/v1/dm/list.so?oid=%1").arg(cid));
    request.setRawHeader("User-Agent", RandomUserAgent());

    auto result = NetResult<DanmakuList>::Alloc();
    auto reply = manager.get(request);


    connect(reply, &QNetworkReply::finished, this, [this, reply, result]() mutable {
        _on_danmakuReplyReady(result, reply);
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
void    BiliClient::_on_danmakuReplyReady(NetResult<DanmakuList> &result, QNetworkReply *reply) {
    Result<DanmakuList> danmakus;

    reply->deleteLater();
    if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) == 200) {
        auto text = reply->readAll();
        danmakus = ParseDanmaku(text);
    }

    result.putResult(danmakus);
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