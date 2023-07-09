#include <QWebEngineUrlRequestInterceptor>
#include <QWebEnginePage>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QImage>
#include <QTimer>
#include "client.hpp"
#include "cache.hpp"
#include "yhdm.hpp"
#include "lxml.hpp"

namespace {

// For craling data
class YhdmVideoSpider final : public QWebEngineUrlRequestInterceptor {
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
        auto url = info.requestUrl().toString();
        if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeImage) {
            qDebug() << "YhdmVideoSpider Block" << info.requestUrl();
            info.block(true);
            return;
        }
        else if (url.endsWith(".m3u8") || url.endsWith(".mp4")) {
            // Got
            resultUrl = info.requestUrl().toString();
            page->triggerAction(QWebEnginePage::Stop);
            page->load(QUrl("about:blank"));
            page.reset();

            deleteLater();
            result.putLater(resultUrl);
        }
        qDebug() << "YhdmVideoSpider " << info.requestUrl();
    }

    QScopedPointer<QWebEnginePage, QScopedPointerDeleteLater> page {
        new QWebEnginePage()
    };
    QString resultUrl;
    NetResult<QString> result; //< Result to notify about
};

class YhdmEpisode final : public Episode {
public:
    QString title() override {
        return _title;
    }
    QString indexTitle() override {
        auto u8 = _title.toUtf8();
        int index;
        if (::sscanf(u8.constData(), u8"第%d集", &index) == 1) {
            return QString::number(index);
        }
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
    VideoInterface *rootInterface() override {
        return &client;
    }


    YhdmClient &client;
    // QStringList _sourceList;
    QString    _title;
    QString    _url;
    QString    _videoUrl;

    YhdmEpisode(YhdmClient &client, const QString &title, const QString &url) 
        : client(client), _title(title), _url(url) { }
};

class YhdmBangumi final : public Bangumi {
public:
    QStringList availableSource() override {
        return QStringList(YHDM_CLIENT_NAME);
    }
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
    VideoInterface *rootInterface() override {
        return &client;
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
                QString url = client.domain() + QString::fromUtf8(node->property("href")).remove(0, 1);
                QString title = QString::fromUtf8(node->content());

                self->episodeList.push_back(qMakePair(title, url));
            }
        }

        return self;
    }

    YhdmBangumi(YhdmClient &client) : client(client) { }
};

class YhdmTimelineEpisode final : public TimelineEpisode {
public:
    YhdmTimelineEpisode(YhdmClient &client, QString title, QString url, QString pubIdx) : client(client), title(title), url(url), pubIdx(pubIdx) { }

    VideoInterface *rootInterface() override {
        return &client;
    }
    QString bangumiTitle() override {
        return title;
    }
    QString pubIndexTitle() override {
        return pubIdx;
    }
    bool    hasCover() override {
        return false;
    }
    NetResult<QImage> fetchCover() override {
        return NetResult<QImage>::AllocWithResult(std::nullopt);
    }
    QStringList       availableSource() override {
        return QStringList(YHDM_CLIENT_NAME);
    }
    NetResult<BangumiPtr> fetchBangumi() override {
        auto r = NetResult<BangumiPtr>::Alloc();
        client.fetchFile(url).then([r, c = std::ref(client)](const Result<QByteArray> &html) mutable {
            if (html) {
                auto doc = LXml::HtmlDocoument::Parse(html.value());
                if (doc) {
                    auto bangumi = YhdmBangumi::ParsePageHtml(c, doc);
                    r.putResult(bangumi);
                    return;
                }
            }
            r.putResult(std::nullopt);
        });
        return r;
    }

    YhdmClient &client;
    QString     title;
    QString     url;
    QString     pubIdx;
};
class YhdmTimelineItem final : public TimelineItem {
public:
    YhdmTimelineItem(YhdmClient &client) : client(client) { }
    QDate date() override {
        return _date;
    }
    int   dayOfWeek() override {
        return _dayOfWeek;
    }
    VideoInterface *rootInterface() override {
        return &client;
    }
    TimelineEpisodeList episodesList() override {
        TimelineEpisodeList r;
        for (auto n = 0; n < titles.size(); n++) {
            r.push_back(std::make_shared<YhdmTimelineEpisode>(client, titles[n], urls[n], pubIndexTitles[n]));
        }
        return r;
    }

    YhdmClient &client;
    QStringList titles;
    QStringList urls; //< Url for play page
    QStringList pubIndexTitles; //< PubIndexTitles for play page like ("第一集")
    int        _dayOfWeek = 0;
    QDate      _date;
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
            url = domain() + QString::fromUtf8(aNode->property("href")).remove(0, 1);
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
    // QNetworkRequest request;
    // request.setUrl(domain());
    // request.setHeader(QNetworkRequest::UserAgentHeader, RandomUserAgent());

    auto result = NetResult<Timeline>::Alloc();
    fetchFile(domain()).then(this, [this, result](const Result<QByteArray> &data) mutable {
        if (!data) {
            result.putResult(std::nullopt);
            return;
        }

        // Begin parse
        auto doc = LXml::HtmlDocoument::Parse(data.value());
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

                item->urls.push_back(domain() + QString::fromUtf8(href).remove(0, 1));
                item->titles.push_back(QString::fromUtf8(title));

                // Find previus span/a
                auto span = a->parent()->findChild("span");
                if (span) {
                    auto subA = span->findChild("a");
                    item->pubIndexTitles.push_back(QString::fromUtf8(subA->content()));
                }

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

    return HttpCacheService::instance()->get(request, manager);
}
NetResult<QImage> YhdmClient::fetchImage(const QString &url) {
    return WrapHttpImagePromise(fetchFile(url));
}
QString YhdmClient::domain() const {
    return urls[0];
}


ZOOD_REGISTER_VIDEO_INTERFACE(YhdmClient);