#include "client.hpp"
#include <QApplication>
#include <QNetworkReply>
#include <QThreadPool>

QByteArray RandomUserAgent() {
    // 创建一个UserAgent数组
    const char * userAgents[] = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.212 Safari/537.36",
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1 Safari/605.1.15",
        "Mozilla/5.0 (iPhone; CPU iPhone OS 14_5_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1 Mobile/15E148 Safari/604.1",
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Ubuntu Chromium/90.0.4430.212 Chrome/90.0.4430.212 Safari/537.36",
        "Mozilla/5.0 (Windows NT 10.0; WOW64; Trident/7.0; rv:11.0) like Gecko",
        "Mozilla/5.0 (iPad; CPU OS 14_5_1 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/14.1 Mobile/15E148 Safari/604.1",
        "Mozilla/5.0 (Android 11; Mobile; rv:88.0) Gecko/88.0 Firefox/88.0",
        "Mozilla/5.0 (compatible; MSIE 9.0; Windows NT 6.1; Trident/5.0)",
        "Opera/9.80 (Windows NT 6.1; WOW64) Presto/2.12.388 Version/12.18",
        "Mozilla/5.0 (Linux; Android 11; SM-G998B Build/RP1A.200720.012; wv) AppleWebKit/537.36 (KHTML, like Gecko) Version/4.0 Chrome/90.0.4430.210 Mobile Safari/537."
    };
    
    // 随机选择一个UserAgent并返回
    int index = ::rand() %  (sizeof(userAgents) / sizeof(void*));
    return userAgents[index];
}

static QList<VideoInterface *(*)()> &GetVideoCreateList() {
    static QList<VideoInterface *(*)()> list;
    return list;
}
static bool VideoInited = false;

QList<VideoInterface*> &GetVideoInterfaceList() {
    static QList<VideoInterface*> list;
    return list;
}
void RegisterVideoInterface(VideoInterface *(*fn)()) {
    if (!VideoInited) {
        // Not inited, just add into list
        GetVideoCreateList().push_back(fn);
    }
    else {
        auto interface = fn();
        GetVideoInterfaceList().push_back(interface);

        interface->setParent(qApp);
    }
}
void InitializeVideoInterface() {
    if (VideoInited) {
        return;
    }
    RegisterBiliClient();
    RegisterYhdmClient();

    VideoInited = true;
    auto &objects = GetVideoInterfaceList();
    for (auto fn : GetVideoCreateList()) {
        auto interface = fn();
        objects.push_back(interface);

        interface->setParent(qApp);
    }
}

NetResult<QByteArray>   WrapQNetworkReply(QNetworkReply *reply) {
    auto result = NetResult<QByteArray>::Alloc();

    QObject::connect(reply, &QNetworkReply::finished, [reply, result]() mutable {
        qDebug() << "GET " 
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

static QThreadPool *decodeThreadPool = nullptr;

NetResult<QImage>       WrapHttpImagePromise(NetResult<QByteArray> prev) {
    auto r = NetResult<QImage>::Alloc();
    prev.then([r](const Result<QByteArray> &b) mutable {
        if (!b) {
            r.putResult(std::nullopt);
            return;
        }
        QThreadPool::globalInstance()->start([r, rawData = b.value()]() mutable {
            auto image = QImage::fromData(rawData);
            if (image.isNull()) {
                r.putResult(std::nullopt, Qt::BlockingQueuedConnection);
                return;
            }
            r.putResult(image, Qt::BlockingQueuedConnection);
        });
    });
    return r;
}