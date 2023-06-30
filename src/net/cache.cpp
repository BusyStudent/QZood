#include <QNetworkReply>
#include <QMutexLocker>
#include <QApplication>
#include <list>
#include "cache.hpp"

class HttpCacheServicePrivate {
public:
    class CacheItem;
    using Index = std::map<QString, CacheItem>::iterator;

    class CacheItem {
        public:
            QByteArray data;
            QDateTime  timestamp;
            int        compressionLevel;

            std::list<Index>::iterator indexInList;
    };

    size_t                        cachedSize = 0;
    size_t                        cachedMaxSize = 50 * 1024 * 1024;
    QMutex                        cacheMutex;
    std::map<QString, CacheItem>  cachedItems;
    std::list<Index>              cachedIndexList; //< List for find the first one added to the list


    inline void putData(const QString &url, QByteArray data, int compressionLevel) {
        if (compressionLevel != HttpCacheService::NoCompression) {
            auto compressed = qCompress(data, compressionLevel);
            if (compressed.size() > data.size()) {
                // Compress does works, just like uncompressed
                compressionLevel = HttpCacheService::NoCompression;
            }
            else {
                data = compressed;
            }
        }
        QMutexLocker locker(&cacheMutex);

        auto iter = cachedItems.find(url);
        if (iter == cachedItems.end()) {
            CacheItem item {
                data,
                QDateTime::currentDateTime(),
                compressionLevel
            };
            auto [iter, ok] = cachedItems.insert(std::make_pair(url, item));
            if (ok)  {
                cachedIndexList.push_back(iter);
                cachedSize += data.size();

                iter->second.indexInList = --cachedIndexList.end();
            }
        }
        else {
            // Replace previous cached
            cachedSize -= iter->second.data.size();
            cachedSize += data.size();

            iter->second.data = data;
            iter->second.timestamp = QDateTime::currentDateTime();
        }

        // Check current cached size
        if (cachedSize > cachedMaxSize) {
            clear();
        }
    }
    inline bool delData(const QString &url) {
        QMutexLocker locker(&cacheMutex);

        auto iter = cachedItems.find(url);
        if (iter != cachedItems.end()) {
            cachedSize -= iter->second.data.size();
            cachedIndexList.erase(iter->second.indexInList);
            cachedItems.erase(iter);
            return true;
        }
        return false;
    }
    inline QByteArray queryData(const QString &url) {
        QMutexLocker locker(&cacheMutex);

        auto iter = cachedItems.find(url);
        if (iter != cachedItems.end()) {
            if (iter->second.compressionLevel == HttpCacheService::NoCompression) {
                return iter->second.data;
            }
            return qUncompress(iter->second.data);
        }
        return QByteArray();
    }

    inline void      clear() {
        qDebug() << "HttpCacheService: current cached size is " << cachedMaxSize / 1024.0 / 1024.0 << " MiB, triggering cleanup";
        for (auto iter = cachedIndexList.begin(); iter != cachedIndexList.end();) {
            if (cachedSize < cachedMaxSize / 2) {
                break;
            }

            auto index = *iter;
            const auto &[key, item] = *index;

            // Del this item from the cache
            cachedSize -= item.data.size();
            cachedItems.erase(index);
            iter = cachedIndexList.erase(iter);
        }
        qDebug() << "HttpCacheService: current cached size is " << cachedMaxSize / 1024.0 / 1024.0 << " MiB, after cleanup";
    }
};

HttpCacheService::HttpCacheService(QObject *parent) : QObject(parent), d(new HttpCacheServicePrivate) {

}
HttpCacheService::~HttpCacheService() {

}
void HttpCacheService::putData(const QString &url, const QByteArray &data, int compresionLevel) {
    compresionLevel = qBound<int>(NoCompression, compresionLevel, BestCompression);
    return d->putData(url, data, compresionLevel);
}
bool HttpCacheService::delData(const QString &url) {
    return d->delData(url);
}
void HttpCacheService::clear() {
    return d->clear();
}
void HttpCacheService::dumpCacheInfo() const {
    qDebug() << "HttpCacheService: current cached size is " << d->cachedMaxSize / 1024.0 / 1024.0 << " MiB";
}
size_t HttpCacheService::currentCachedSize() const {
    return d->cachedSize;
}
QByteArray HttpCacheService::queryData(const QString &url) const {
    return d->queryData(url);
}

NetResult<QByteArray> HttpCacheService::get(const QNetworkRequest &request, QNetworkAccessManager &manager) {
    auto cache = queryData(request.url());
    if (cache.isNull()) {
        return wrapReply(manager.get(request));
    }
    qDebug() << "GET " << request.url() << " Using cache";
    return NetResult<QByteArray>::AllocWithResult(cache);
}
NetResult<QByteArray> HttpCacheService::post(const QNetworkRequest &request, QNetworkAccessManager &manager, const QByteArray &data) {
    auto cache = queryData(request.url());
    if (cache.isNull()) {
        return wrapReply(manager.post(request, data));
    }
    qDebug() << "POST " << request.url() << " Using cache";
    return NetResult<QByteArray>::AllocWithResult(cache);
}
NetResult<QByteArray> HttpCacheService::wrapReply(QNetworkReply *reply) {
    auto r = NetResult<QByteArray>::Alloc();
    QObject::connect(reply, &QNetworkReply::finished, this, [this, r, reply]() mutable {
        reply->deleteLater();

        const char *prefix = nullptr;
        switch (reply->operation()) {
            case QNetworkAccessManager::GetOperation : prefix = "GET"; break;
            case QNetworkAccessManager::PostOperation : prefix = "POST"; break;
            default : qFatal() << "Impossible Operation in HttpCacheService::wrapReply"; break;
        }

        auto contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
        auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        auto data = reply->readAll();

        qDebug() << prefix << " " << reply->url() << "Status code : " << statusCode << " " << ((statusCode == 200) ? "OK" : "FAILED");
        
        if (statusCode == 200) {
            CompressionLevel level = NoCompression;
            r.putResult(data); //< Callback

            // Check mime type
            if (!contentType.startsWith("image/")) {
                level = BestCompression;
            }

            putData(reply->request().url().toString(), data, level); //< Cache data
        }
        else {
            r.putResult(std::nullopt);
        }
    });
    return r;
}


static HttpCacheService *httpCacheService = nullptr;
HttpCacheService *HttpCacheService::instance() {
    if (!httpCacheService) {
        httpCacheService = new HttpCacheService(qApp);
    }
    return httpCacheService;
}