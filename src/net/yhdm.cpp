#if defined(QZOOD_WEBENGINE_CORE)
#include <QWebEngineUrlRequestInterceptor>
#include <QWebEnginePage>
#endif

#include <QRandomGenerator>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QImage>
#include <QTimer>
#include "client.hpp"
#include "cache.hpp"
#include "yhdm.hpp"
#include "lxml.hpp"

namespace {

#if defined(QZOOD_WEBENGINE_CORE)
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
        else if ((url.contains(".m3u8") && !url.contains("player/")) || url.endsWith(".mp4")) {
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
#endif

// Directly use yhdm javascript algo
class YhdmJSSpider : public QObject {
public:
    YhdmJSSpider(YhdmClient &client, NetResult<QString> r, const QString &url) : 
        client(client), result(r), url(url), requestUrl(GenerateUrl(url)) 
    {
        sendReuqest(RandomUserAgent());
    }
    void sendReuqest(const QByteArray &us) {
        QNetworkRequest req;
        req.setUrl(requestUrl);
        req.setHeader(QNetworkRequest::UserAgentHeader, us);
        req.setRawHeader("Referer", url.toUtf8());

        auto reply = client.networkManager().get(req);
        connect(reply, &QNetworkReply::finished, [this, reply]() mutable {
            reply->deleteLater();
            if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute) != 200) {
                qDebug() << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
                fail();
                return;
            }
            auto data = reply->readAll();
            if (data.startsWith("ipchk")) {
                // We need restart
                retryLeft -= 1;
                if (retryLeft == 0) {
                    fail();
                    return;
                }
                sendReuqest(reply->request().header(QNetworkRequest::UserAgentHeader).toString().toUtf8());
                return;
            }
            // Try parse
            auto json = ParseData(QString::fromUtf8(data));
            // Try parse json
            auto doc = QJsonDocument::fromJson(json.toUtf8());
            if (doc.isEmpty()) {
                fail();
                return;
            }
            resultUrl = doc["vurl"].toString();
            if (resultUrl.isEmpty() || resultUrl == QStringLiteral("404.mp4")) {
                fail();
                return;
            }
            resultUrl = QUrl::fromPercentEncoding(resultUrl.toUtf8());
            qDebug() << resultUrl;
            if (resultUrl.startsWith("/")) {
                
            }
            result.putResult(resultUrl);
            deleteLater();
        });
    }
    void fail() {
        qDebug() << "Failed to Get for" << url;
        result.putResult(std::nullopt);
        deleteLater();
    }

    static QString GenerateUrl(const QString &url) {
        QStringList parts = url.split("/");

        if (parts.size() < 2) {
            return "";
        }

        QStringList aidParts = parts.last().split("-");

        if (aidParts.size() != 3) {
            return "";
        }

        QString aid = aidParts.at(0);
        QString playindex = aidParts.at(1);
        QString epindex = aidParts.at(2).split(".").first();

        double randomValue = QRandomGenerator::global()->generateDouble();

        QString u = "playurl?aid=" + aid + "&playindex=" + playindex + "&epindex=" + epindex + "&r=" + QString::number(randomValue);

        return "https://" + QUrl(url).host() + '/' + u;

    }
    static QString ParseData(const QString &data) {
        QString output;
        const int magic = 1561;
        const int len = data.length();
        for (int cur = 0; cur < len; cur += 2) {
            QString hex = QString(data[cur]) + QString(data[cur + 1]);
            bool ok;
            int ret = hex.toInt(&ok, 16);
            if (ok) {
                ret = (((ret + 1048576) - magic) - (((len / 2) - 1) - (cur / 2))) % 256;
                output = QString(QChar(ret)) + output;
            }
        }
        return output;
    }
    YhdmClient &client;
    QString url; //< Start Url
    QString requestUrl;
    QString resultUrl;
    NetResult<QString> result;
    int retryLeft = 3;
};

class YhdmEpisode final : public Episode {
public:
    QString title() override {
        return _title;
    }
    QString indexTitle() override {
        return toIndexTitle(_title);
    }
    QStringList sourcesList() override {
        int idx = 1;
        QStringList ret;
        for (const auto &s : _urls) {
            ret.push_back(u8"线路" + QString::number(idx));
            idx += 1;
        }
        return ret;
    }
    QString     recommendedSource() override {
        return QStringLiteral("线路1");
    }
    NetResult<QString> fetchVideo(const QString &sourceString) override {
        if (!_videoUrl.isEmpty()) {
            return NetResult<QString>::AllocWithResult(_videoUrl);
        }
        int idx;
        if (::sscanf(sourceString.toUtf8().constData(), "线路%d",&idx) != 1) {
            return NetResult<QString>::AllocWithResult(_videoUrl);
        }
        idx -= 1; //< To program Index
        if (idx >= _urls.size()) {
            return NetResult<QString>::AllocWithResult(_videoUrl);
        }
        auto result = NetResult<QString>::Alloc();
#if     defined(QZOOD_WEBENGINE_CORE)
        auto tools = new YhdmVideoSpider(result, _urls[idx]);
#else
        auto tools = new YhdmJSSpider(client, result, _urls[idx]);
#endif
        return result;
    }
    VideoInterface *rootInterface() override {
        return &client;
    }

    static QString toIndexTitle(const QString &_title) {
        auto u8 = _title.toUtf8();
        int index;
        if (::sscanf(u8.constData(), u8"第%d集", &index) == 1) {
            return QString::number(index);
        }
        return _title;
    }


    YhdmClient &client;
    // QStringList _sourceList;
    QStringList _urls;
    QString    _title;
    QString    _videoUrl;

    YhdmEpisode(YhdmClient &client, const QString &title, const QStringList &urls) 
        : client(client), _title(title), _urls(urls) { }
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
                for (const auto &[title, urls] : episodeList) {
                    list.push_back(std::make_shared<YhdmEpisode>(client, title, urls));
                }
                result.putResult(list);
            });
            return result;
        }
        EpisodeList list;
        for (const auto &[title, urls] : episodeList) {
            list.push_back(std::make_shared<YhdmEpisode>(client, title, urls));
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

    QList<QPair<QString, QStringList>> episodeList;
    //< List for source

    static RefPtr<YhdmBangumi> ParsePageHtml(YhdmClient &client, const LXml::HtmlDocoument &doc) {
        auto self = std::make_shared<YhdmBangumi>(client);

        //div[@class='thumb l']/img 
        LXml::XPathContext ctxt(doc);

        // Get cover url
        auto imgNode = ctxt.eval("//div[@class='thumb l']/img");
        if (imgNode.nodeTab()) {
            self->_cover = imgNode.nodeTab()[0]->property("src");
        }
        qDebug() << imgNode;

        //div[@class='info']
        // For description
        auto descriptionNode = ctxt.eval("//div[@class='info']");
        if (descriptionNode.nodeTab()) {
            self->_description = descriptionNode.nodeTab()[0]->content();
        }

        //div[@class='rate r']/h1
        // title
        auto titleNode = ctxt.eval("//div[@class='rate r']/h1");
        if (titleNode.nodeTab()) {
            self->_title = titleNode.nodeTab()[0]->content();
        }

        // Urls
        //div[@class='movurl']/ul/li/a
        auto urlsNode = ctxt.eval("//div[@class='movurl']/ul/li/a");
        std::map<QString, QStringList> maps; //< Title & Url Pairs
        if (urlsNode.nodeTab()) {
            for (auto node : urlsNode) {
                QString url = client.domain() + node->property("href").remove(0, 1);
                QString title = node->content();

                // self->episodeList.push_back(qMakePair(title, url));
                maps[title].push_back(url);
            }
        }
        // Generate pair here
        for (const auto &[title, urls] : maps) {
            self->episodeList.push_back(qMakePair(title, urls));
        }
        // Sort the episodeList
        self->sortEpisodeList();


        return self;
    }
    void sortEpisodeList() {
        decltype(episodeList) sortedList;
        decltype(episodeList) leftList;

        for (const auto &t : episodeList) {
            bool ok;
            YhdmEpisode::toIndexTitle(t.first).toInt(&ok);
            if (ok) {
                sortedList.push_back(t);
            }
            else {
                leftList.push_back(t);
            }
        }

        std::sort(sortedList.begin(), sortedList.end(), [](const auto &a, const auto &b) {
            return YhdmEpisode::toIndexTitle(a.first).toInt() < YhdmEpisode::toIndexTitle(b.first).toInt();
        });

        sortedList.append(leftList);
        episodeList = sortedList;
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
    manager.setCookieJar(new QNetworkCookieJar(this));
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

    // QString url = domain() + "search/" + name;
    // QUrlQuery query;
    // query.addQueryItem("m", "search");
    // query.addQueryItem("c", "index");
    // query.addQueryItem("a", "init");
    // query.addQueryItem("q", name);
    QString url = domain() + "s_all?ex=1&kw=" + name;

    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, RandomUserAgent());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // auto reply = manager.post(request, query.query().toUtf8());
    auto reply = manager.get(request);
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
                description = descriptionNode->content();
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
            url = domain() + aNode->property("href").remove(0, 1);
            cover = imgNode->property("src");
            title = imgNode->property("alt");

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
            item->_date = currentDate.addDays(current - currentDayOfWeek);

            auto aNodeset = ctxt.eval(node, ".//li/a");
            for (auto a : aNodeset) {
                auto href = a->property("href");
                auto title = a->property("title");

                item->urls.push_back(domain() + href.remove(0, 1));
                item->titles.push_back(title);

                // Find previus span/a
                auto span = a->parent()->findChild("span");
                if (span) {
                    auto subA = span->findChild("a");
                    item->pubIndexTitles.push_back(subA->content());
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
bool    YhdmClient::hasSupport(int what)  {
    switch (what) {
        case SearchSupport:
        case TimelineSupport:
            return true;
        default:
            return false;
    }
}


ZOOD_REGISTER_VIDEO_INTERFACE(YhdmClient);