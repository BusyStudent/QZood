#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QDateTime>
#include <QObject>
#include <QMutex>
#include <map>

#include "promise.hpp"

class HttpCacheServicePrivate;
class HttpCacheService : public QObject {
    Q_OBJECT
    public:
        enum CompressionLevel {
            NoCompression = -2,
            AutoCompression = -1,
            FastCompression =  0,
            BestCompression =  9,
        };

        static HttpCacheService *instance();

        NetResult<QByteArray> get(const QNetworkRequest &request, QNetworkAccessManager &manager);
        NetResult<QByteArray> post(const QNetworkRequest &request, QNetworkAccessManager &manager, const QByteArray &data);
        NetResult<QByteArray> wrapReply(QNetworkReply *reply);

        void putData(const QString &url, const QByteArray &data, int compressionLevel = AutoCompression);
        void putData(const QUrl &url, const QByteArray &data, int compressionLevel = AutoCompression);
        bool delData(const QString &url);
        bool delData(const QUrl &url);
        QByteArray queryData(const QString &url) const;
        QByteArray queryData(const QUrl &url) const;

        size_t currentCachedSize() const;
        void   dumpCacheInfo() const;
        void   clear();
    private:
        HttpCacheService(QObject *parent = nullptr);
        ~HttpCacheService();

        QScopedPointer<HttpCacheServicePrivate> d;
};

inline void HttpCacheService::putData(const QUrl &url, const QByteArray &data, int compressionLevel) {
    return putData(url.toString(), data, compressionLevel);
}
inline bool HttpCacheService::delData(const QUrl &url) {
    return delData(url.toString());
}
inline QByteArray HttpCacheService::queryData(const QUrl &url) const {
    return queryData(url.toString());
}