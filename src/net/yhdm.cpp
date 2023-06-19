#include <QWebEngineUrlRequestInterceptor>
#include <QWebEnginePage>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QImage>
#include <QTimer>
#include "client.hpp"
#include "yhdm.hpp"
#include "lxml.hpp"

namespace {

// For craling data
class YhdmVideoSpider : public QWebEngineUrlRequestInterceptor {
public:
    YhdmVideoSpider(NetResult<QString> r, const QString &url) : result(r) {
        page->setUrlRequestInterceptor(this);
        page->load(url);

        QTimer::singleShot(std::chrono::seconds(10), this, [this]() {
            qDebug() << "YhdmVideoSpider timeout";
            result.putLater(std::nullopt);
            deleteLater();
        });
    }

    void interceptRequest(QWebEngineUrlRequestInfo &info) override {
        if (info.requestUrl().toString().endsWith(".m3u8")) {
            // Got
            resultUrl = info.requestUrl().toString();
            page->triggerAction(QWebEnginePage::Stop);
            page->load(QUrl("about:blank"));
            page.reset();

            result.putLater(resultUrl);
            deleteLater();
        }
        qDebug() << "YhdmVideoSpider " << info.requestUrl();
    }

    QScopedPointer<QWebEnginePage, QScopedPointerDeleteLater> page {
        new QWebEnginePage()
    };
    QString resultUrl;
    NetResult<QString> result; //< Result to notify about
};

class YhdmEpisode : public Episode {
public:
    QString title() override {
        return _title;
    }
    QStringList sourcesList() override {
        return QStringList(" ");
    }
    QString     recommendedSource() override {
        return QString(" ");
    }
    NetResult<QString> fetchVideo(const QString &sourceString) override {
        if (!_videoUrl.isEmpty()) {
            return NetResult<QString>::AllocWithResult(_videoUrl);
        }
        auto result = NetResult<QString>::Alloc();
        auto tools = new YhdmVideoSpider(result, _url);
        return result;
    }


    YhdmClient &client;
    // QStringList _sourceList;
    QString    _title;
    QString    _url;
    QString    _videoUrl;

    YhdmEpisode(YhdmClient &client, const QString &title, const QString &url) 
        : client(client), _title(title), _url(url) { }
};

class YhdmBangumi : public Bangumi {
public:
    // QStringList availableSource() override {
    //     return QStringList(YHDM_CLIENT_NAME);
    // }
    QString     description() override {
        return _description;
    }
    QString     title() override {
        return _title;
    }
    NetResult<QImage> fetchCover() override {
        return client.fetchImage(_cover);
    }
    NetResult<EpisodeList> fetchEpisodes() override {
        if (episodeList.empty()) {
            auto result = NetResult<EpisodeList>::Alloc();
            client.fetchFile(_url)
                .then([this, result, holder = shared_from_this()](const Result<QByteArray> &html) mutable {
                
                if (!html) {
                    result.putResult(std::nullopt);
                    return;
                }
                auto doc = LXml::HtmlDocoument::Parse(html.value());
                if (!doc) {
                    result.putResult(std::nullopt);
                    return;
                }
                auto r = ParsePageHtml(client, doc);
                episodeList = r->episodeList;

                // Parse done
                EpisodeList list;
                for (const auto &[title, url] : episodeList) {
                    list.push_back(std::make_shared<YhdmEpisode>(client, title, url));
                }
                result.putResult(list);
            });
            return result;
        }
        EpisodeList list;
        for (const auto &[title, url] : episodeList) {
            list.push_back(std::make_shared<YhdmEpisode>(client, title, url));
        }
        return NetResult<EpisodeList>::AllocWithResult(list);
    }

    YhdmClient &client;
    QString _cover;
    QString _title;
    QString _description;
    QString _url; //< Page url

    QList<QPair<QString, QString>> episodeList;
    //< List for source

    static RefPtr<YhdmBangumi> ParsePageHtml(YhdmClient &client, const LXml::HtmlDocoument &doc) {
        auto self = std::make_shared<YhdmBangumi>(client);

        //div[@class='thumb l']/img 
        LXml::XPathContext ctxt(doc);

        // Get cover url
        auto imgNode = ctxt.eval("//div[@class='thumb l']/img");
        if (imgNode.nodeTab()) {
            self->_cover = QString::fromUtf8(imgNode.nodeTab()[0]->property("src"));
        }

        //div[@class='info']
        // For description
        auto descriptionNode = ctxt.eval("//div[@class='info']");
        if (descriptionNode.nodeTab()) {
            self->_description = QString::fromUtf8(descriptionNode.nodeTab()[0]->content());
        }

        //div[@class='rate r']/h1
        // title
        auto titleNode = ctxt.eval("//div[@class='rate r']/h1");
        if (titleNode.nodeTab()) {
            self->_title = QString::fromUtf8(titleNode.nodeTab()[0]->content());
        }

        // Urls
        //div[@class='movurl']/ul/li/a
        auto urlsNode = ctxt.eval("//div[@class='movurl']/ul/li/a");
        if (urlsNode.nodeTab()) {
            for (auto node : urlsNode) {
                QString url = client.domain() + QString::fromUtf8(node->property("href")).removeFirst();
                QString title = QString::fromUtf8(node->content());

                self->episodeList.push_back(qMakePair(title, url));
            }
        }

        return self;
    }

    YhdmBangumi(YhdmClient &client) : client(client) { }
};
class YhdmTimelineItem : public TimelineItem {
public:
    YhdmTimelineItem(YhdmClient &client) : client(client) { }
    QDate date() override {
        return _date;
    }
    int   dayOfWeek() override {
        return _dayOfWeek;
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
        requestLeft = urls.size();

        for (const auto &u : urls) {
            client.fetchFile(u).then([this, r, holder = shared_from_this()](const Result<QByteArray> &html) mutable {
                if (html) {
                    auto doc = LXml::HtmlDocoument::Parse(html.value());
                    if (doc) {
                        outList.push_back(YhdmBangumi::ParsePageHtml(client, doc));
                    }
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


    YhdmClient &client;
    QStringList titles;
    QStringList urls; //< Url for play page
    int        _dayOfWeek = 0;
    QDate      _date;

    // Fetch bangumi list
    int         requestLeft = -1;
    BangumiList outList;
    NetResult<BangumiList> pendingPromise;
};

}

YhdmClient::YhdmClient(QObject *parent)  {
    setParent(parent);
}
YhdmClient::~YhdmClient() {
    TimelineItemPtr ptr;
}
QString YhdmClient::name() {
    return YHDM_CLIENT_NAME;
}
NetResult<BangumiList> YhdmClient::searchBangumi(const QString &name) {
    auto result = NetResult<BangumiList>::Alloc();

    QString url = domain() + "search/" + name;
    QUrlQuery query;
    query.addQueryItem("m", "search");
    query.addQueryItem("c", "index");
    query.addQueryItem("a", "init");
    query.addQueryItem("q", name);

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, RandomUserAgent());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    auto reply = manager.post(request, query.query().toUtf8());
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, result]() mutable {
        reply->deleteLater();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 200) {
            result.putResult(std::nullopt);
            return;
        }
        auto data = reply->readAll();
        auto doc = LXml::HtmlDocoument::Parse(data);
        if (!doc) {
            result.putResult(std::nullopt);
            return;
        }

        //div[@class='lpic']/ul/li
        LXml::XPathContext ctxt(doc);
        auto nodes = ctxt.eval("//div[@class='lpic']/ul/li");
        BangumiList list;
        for (auto node : nodes) {
            auto descriptionNode = node->findChild("p");
            QString description;
            if (descriptionNode) {
                description = QString::fromUtf8(descriptionNode->content());
            }
            auto aNode = node->findChild("a");
            QString title;
            QString cover;
            QString url;   //< Url for play page
            if (!aNode) {
                qWarning() << "can not find aNode";
                result.putResult(std::nullopt);
                return;
            }
            auto imgNode = aNode->findChild("img");
            if (!aNode) {
                qWarning() << "can not find imgNode";
                result.putResult(std::nullopt);
                return;
            }
            url = domain() + QString::fromUtf8(aNode->property("href")).removeFirst();
            cover = QString::fromUtf8(imgNode->property("src"));
            title = QString::fromUtf8(imgNode->property("alt"));

            auto ban = std::make_shared<YhdmBangumi>(*this);
            ban->_url = url;
            ban->_title = title;
            ban->_cover = cover;
            ban->_description = description;

            list.push_back(ban);
        }
        result.putResult(list);
    });

    return result;
}
NetResult<Timeline>    YhdmClient::fetchTimeline() {
    QNetworkRequest request;
    request.setUrl(domain());
    request.setHeader(QNetworkRequest::UserAgentHeader, RandomUserAgent());

    auto reply = manager.get(request);
    auto result = NetResult<Timeline>::Alloc();
    QObject::connect(reply, &QNetworkReply::finished, this, [this, reply, result]() mutable {
        reply->deleteLater();
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 200) {
            result.putResult(std::nullopt);
            return;
        }
        auto data = reply->readAll();

        // Begin parse
        auto doc = LXml::HtmlDocoument::Parse(data);
        if (!doc) {
            result.putResult(std::nullopt);
            return;
        }
        LXml::XPathContext ctxt(doc);
        auto elems = ctxt.eval("//div[@class='tlist']/ul");
        if (!elems || !elems.isNodeset()) {
            result.putResult(std::nullopt);
            return;
        }
        auto currentDate = QDate::currentDate();
        int n = elems.nodeCount();
        int current = 1; //< Day 1
        int currentDayOfWeek = currentDate.dayOfWeek();

        if (n != 7) {
            qWarning() << "YhdmClient fetchTimeline it should has 7 elements";
        }

        Timeline timeline;
        for (auto node : elems) {
            // Get sub
            auto item = std::make_shared<YhdmTimelineItem>(*this); //< As a day
            item->_dayOfWeek = current;
            item->_date = currentDate.addDays(currentDayOfWeek - current);

            auto aNodeset = ctxt.eval(node, ".//li/a");
            for (auto a : aNodeset) {
                auto href = a->property("href");
                auto title = a->property("title");

                item->urls.push_back(domain() + QString::fromUtf8(href).removeFirst());
                item->titles.push_back(QString::fromUtf8(title));

                qDebug() << "href : " << item->urls.back() << " title : " << item->titles.back();
            }

            timeline.push_back(item);
            current += 1;
        }

        auto nowDay = QDate::currentDate();
        int year = nowDay.year();
        int month = nowDay.month();

        // for (const auto &v : timeline) {
        //     auto item = v->as<YhdmTimelineItem>();

        // }


        result.putResult(timeline);
    });

    return result;
}
NetResult<QByteArray> YhdmClient::fetchFile(const QString &url) {
    QNetworkRequest request;

    // Mark request
    request.setUrl(url);
    request.setRawHeader("User-Agent", RandomUserAgent());

    return WrapQNetworkReply(manager.get(request));
}
NetResult<QImage> YhdmClient::fetchImage(const QString &url) {
    return WrapHttpImagePromise(fetchFile(url));
}
QString YhdmClient::domain() const {
    return urls[0];
}


ZOOD_REGISTER_VIDEO_INTERFACE(YhdmClient);