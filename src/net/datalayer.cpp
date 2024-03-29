#include <QApplication>
#include "datalayer.hpp"
#include "bilibili.hpp"

namespace {
    
template <typename T, typename Client, typename Class, typename ...TArgs, typename ...Args>
NetPromise<QList<T> > WaitForMultiClient(
    const QList<Client> & clientList, 
    NetResult<T> (Class::*method)(TArgs ...), 
    Args &&...args
) 
{
    struct Data {
        QList<T> collectedList;
        int counter = 0;
    };
    auto r = NetPromise<QList<T> >::Alloc();
    auto data = std::make_shared<Data>();
    data->counter = clientList.size();

    for (const auto &client : clientList) {
        ((*client).*method)(std::forward<Args>(args)...).then([r, data](const Result<T> &d) mutable {
            data->counter -= 1;
            if (d) {
                data->collectedList.push_back(d.value());
            }
            if (data->counter == 0) {
                r.putResult(data->collectedList);
            }
        });
    }

    return r;
};


class MergedEpisode final : public Episode {
public:
    MergedEpisode(const EpisodeList &ep) : episodeList(ep)  {}

    QString title() override {
        return episodeList[0]->title();
    }
    QString indexTitle() override {
        return episodeList[0]->indexTitle();
    }
    QString longTitle() override {
        return episodeList[0]->longTitle();
    }
    NetResult<QImage> fetchCover() override {
        return episodeList[0]->fetchCover();
    }
    QStringList sourcesList() override {
        QStringList list;
        for (const auto &ep : episodeList) {
            for (const auto &epSource : ep->sourcesList()) {
                list.push_back(QString("%1 %2").arg(ep->rootInterface()->name(), epSource));
            }
        }
        return list;
    }
    QStringList danmakuSourceList() override {
        QStringList list;
        for (const auto &ep : episodeList) {
            for (const auto &epSource : ep->danmakuSourceList()) {
                list.push_back(QString("%1 %2").arg(ep->rootInterface()->name(), epSource));
            }
        }
        return list;
    }
    NetResult<QString> fetchVideo(const QString &sourceString) override {
        for (const auto &ep : episodeList) {
            auto name = ep->rootInterface()->name();
            if (sourceString.startsWith(name)) {
                // ADD 1 because space
                return ep->fetchVideo(sourceString.sliced(name.length() + 1));
            }
        }
        return NetResult<QString>::AllocWithResult(std::nullopt);
    }
    NetResult<DanmakuList> fetchDanmaku(const QString &sourceString) override {
        for (const auto &ep : episodeList) {
            auto name = ep->rootInterface()->name();
            if (sourceString.startsWith(name)) {
                // ADD 1 because space
                return ep->fetchDanmaku(sourceString.sliced(name.length() + 1));
            }
        }
        return NetResult<DanmakuList>::AllocWithResult(std::nullopt);
    }

    VideoInterface *rootInterface() override {
        return DataService::instance();
    }

    EpisodeList episodeList;
};
class MergedBangumi final : public Bangumi {
public:
    MergedBangumi(const BangumiList &b) : bangumiList(b) {}

    QStringList availableSource() override {
        QStringList l;
        for (const auto &b : bangumiList) {
            l.append(b->availableSource());
        }

        return l;
    }
    QString     description() override {
        return bangumiList[0]->description();
    }
    QString     title() override {
        return bangumiList[0]->title();
    }

    NetResult<QImage> fetchCover() override {
        return bangumiList[0]->fetchCover();
    }
    NetResult<EpisodeList> fetchEpisodes() override {
        if (requestLeft == 0) {
            return NetResult<EpisodeList>::AllocWithResult(resultEpisodeList);
        }
        requestLeft = bangumiList.size();
        auto r = NetResult<EpisodeList>::Alloc();
        for (const auto &b : bangumiList) {
            b->fetchEpisodes().then([this, r, holder = shared_from_this()](const Result<EpisodeList> &e) mutable {
                requestLeft -= 1;
                if (e) {
                    collectedList.push_back(e.value());
                }
                if (requestLeft == 0) {
                    generateList();

                    r.putResult(resultEpisodeList);
                }
            });
        }
        return r;
    }
    VideoInterface *rootInterface() override {
        return DataService::instance();
    }
    void generateList() {
        std::map<QString, EpisodeList> map;
        for (const auto &list : collectedList) {
            for (const auto &b : list) {
                map[b->indexTitle()].push_back(b);
            }
        }

        for (const auto &[key, list] : map) {
            resultEpisodeList.push_back(std::make_shared<MergedEpisode>(list));
        }
        EpisodeList sortedList;
        EpisodeList lastList;
        for (const auto &t : resultEpisodeList) {
            bool ok;
            t->indexTitle().toInt(&ok);
            if (ok) {
                sortedList.push_back(t);
            }
            else {
                lastList.push_back(t);
            }
        }

        std::sort(sortedList.begin(), sortedList.end(), [](const EpisodePtr &a, const EpisodePtr &b) {
            return a->indexTitle().toInt() < b->indexTitle().toInt();
        });

        sortedList.append(lastList);
        resultEpisodeList = sortedList;
    }

    BangumiList bangumiList;

    EpisodeList resultEpisodeList;
    int         requestLeft = -1;
    QList<EpisodeList> collectedList;
};
class MergedTimelineEpisode final : public TimelineEpisode {
public:
    MergedTimelineEpisode(const TimelineEpisodeList &t) : episodesList(t) { }

    QString bangumiTitle() override {
        return episodesList[0]->bangumiTitle();
    }
    QString pubIndexTitle() override {
        return episodesList[0]->pubIndexTitle();
    }
    bool    hasCover() override {
        return subEpisodeHasCoverIndex() >= 0;
    }
    NetResult<QImage> fetchCover() override {
        if (!hasCover()) {
            return NetResult<QImage>::AllocWithResult(std::nullopt);
        }
        return episodesList[subEpisodeHasCoverIndex()]->fetchCover();
    }
    NetResult<BangumiPtr> fetchBangumi() override {
        // TODO :
        auto r = NetResult<BangumiPtr>::Alloc();
        WaitForMultiClient(episodesList, &TimelineEpisode::fetchBangumi)
            .then([r](const QList<BangumiPtr> &list) mutable {
            
            if (list.isEmpty()) {
                r.putResult(std::nullopt);
                return;
            }

            r.putResult(std::make_shared<MergedBangumi>(list));
        });
        return r;
    }
    VideoInterface *rootInterface() override {
        return DataService::instance();
    }
    QStringList     availableSource() override {
        QStringList l;
        for (const auto &b : episodesList) {
            l.append(b->availableSource());
        }
        return l;
    }
    int subEpisodeHasCoverIndex() {
        if (hasCoverIndex == -2) {
            int idx = 0;
            for (const auto &each : episodesList) {
                if (each->hasCover()) {
                    hasCoverIndex = idx;
                    return hasCoverIndex;
                }
            }
            hasCoverIndex = -1;
        }
        return hasCoverIndex;
    }

    TimelineEpisodeList episodesList;
    int                 hasCoverIndex = -2; //< -2 means unknown
};
class MergeTimelineItem final : public TimelineItem {
public:
    MergeTimelineItem(const QList<TimelineItemPtr> &t) : timelineItems(t) { }


    QDate date() override {
        return timelineItems[0]->date();
    }
    int   dayOfWeek() override {
        return timelineItems[0]->dayOfWeek();
    }
    VideoInterface *rootInterface() override {
        return DataService::instance();
    }
    TimelineEpisodeList episodesList() override {
        // Do merge
        std::map<QString, TimelineEpisodeList> map;
        for (const auto &item : timelineItems) {
            for (const auto &b : item->episodesList()) {
                map[b->bangumiTitle()].push_back(b);
            }
        }
        TimelineEpisodeList resultList;
        for (const auto &[key, list] : map) {
            resultList.push_back(std::make_shared<MergedTimelineEpisode>(list));
        }
        return resultList;
    }

    QList<TimelineItemPtr> timelineItems;
};

class DataServicePrivate final : public DataService {
public:
    DataServicePrivate(QObject *parent) {
        setParent(parent);
        InitializeVideoInterface();

        for (auto v : GetVideoInterfaceList()) {
            if (v->hasSupport(SuggestionsSupport)) {
                suggestionProvider = v;
                break;
            }
        }
    }

    NetResult<Timeline> fetchTimeline() override {
        auto r = NetResult<Timeline>::Alloc();

        WaitForMultiClient(GetVideoInterfaceList(), &VideoInterface::fetchTimeline)
            .then([r](const QList<Timeline> &collectedList) mutable {
                // Do merge
                std::map<QDate, QList<TimelineItemPtr>> map;
                for (const auto &list : collectedList) {
                    for (const auto &b : list) {
                        map[b->date()].push_back(b);
                    }
                }
                Timeline resultList;
                for (const auto &[date, list] : map) {
                    resultList.push_back(std::make_shared<MergeTimelineItem>(list));
                }
                r.putResult(resultList);

            }
        );

        return r;
    }
    NetResult<BangumiList> searchBangumi(const QString &what) override {
        auto r = NetResult<BangumiList>::Alloc();

        WaitForMultiClient(GetVideoInterfaceList(), &VideoInterface::searchBangumi, what)
            .then([r](const QList<BangumiList> &collectedList) mutable {
                // Do merge
                std::map<QString, BangumiList> map;
                for (const auto &list : collectedList) {
                    for (const auto &b : list) {
                        map[b->title()].push_back(b);
                    }
                }
                BangumiList resultList;
                for (const auto &[key, list] : map) {
                    resultList.push_back(std::make_shared<MergedBangumi>(list));
                }
                r.putResult(resultList);

            }
        );

        return r;
    }
    NetResult<QStringList> fetchSearchSuggestions(const QString &what) override {
        if (suggestionProvider) {
            return suggestionProvider->fetchSearchSuggestions(what);
        }
        return NetResult<QStringList>::AllocWithResult(std::nullopt);
    }
    QString name() override {
        return QStringLiteral("DataService");
    }
    bool hasSupport(int) override {
        return true;
    }


    QMap<QString, QString> strDict; //< Map 
    VideoInterface        *suggestionProvider = nullptr;
};

static DataServicePrivate *dataService = nullptr;




}


DataService *DataService::instance() {
    if (!dataService) {
        dataService = new DataServicePrivate(qApp);
    }
    return dataService;
}
