#include <QApplication>
#include "datalayer.hpp"
#include "bilibili.hpp"

namespace {
    
class MergedEpisode : public Episode {
public:
    MergedEpisode(const EpisodeList &ep) : episodeList(ep)  {}

    QString title() override {
        return episodeList[0]->title();
    }
    QString indexTitle() override {
        return episodeList[0]->title();
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
class MergedBangumi : public Bangumi {
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
class MergeTimelineItem : public TimelineItem {
public:
    MergeTimelineItem(const QList<TimelineItemPtr> &t) : timelineItems(t) { }

    VideoInterface *rootInterface() override {
        return DataService::instance();
    }

    QList<TimelineItemPtr> timelineItems;
};

class DataServicePrivate : public DataService {
public:
    DataServicePrivate(QObject *parent) {
        setParent(parent);
        InitializeVideoInterface();
    }

    NetResult<Timeline> fetchTimeline() override {
        return GetVideoInterfaceList()[0]->fetchTimeline();
    }
    NetResult<BangumiList> searchBangumi(const QString &what) override {
        struct Data {
            QList<BangumiList> collectedList;
            int counter = 0;
        };
        auto data = std::make_shared<Data>();
        auto r = NetResult<BangumiList>::Alloc();
        data->counter = GetVideoInterfaceList().size();

        for (const auto &client : GetVideoInterfaceList()) {
            client->searchBangumi(what).then([r, data](const Result<BangumiList> &b) mutable {
                data->counter -= 1;
                if (b) {
                    data->collectedList.push_back(b.value());
                }
                if (data->counter == 0) {
                    // Do merge
                    std::map<QString, BangumiList> map;
                    for (const auto &list : data->collectedList) {
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
            });
        }

        return r;
    }
    QString name() override {
        return "DataService";
    }


    QMap<QString, QString> strDict; //< Map 
};

static DataServicePrivate *dataService = nullptr;




}


DataService *DataService::instance() {
    if (!dataService) {
        dataService = new DataServicePrivate(qApp);
    }
    return dataService;
}
