#include <QApplication>
#include "datalayer.hpp"
#include "bilibili.hpp"

namespace {
namespace DataLayer {

}
}

class DataServicePrivate : public DataService, public QObject {
    public:
        DataServicePrivate(QObject *parent) : QObject(parent) { }

        NetResultPtr<Timeline> fetchTimeline() override;


        QMap<QString, QString> strDict; //< Map 
        BiliClient             biliClient;
};

static DataServicePrivate *dataService = nullptr;

class BangumiPrivate     : public Bangumi {
    public:
        QStringList availableSource() override;
        QString     description() override;
        QString     title() override;
        NetResult<QImage> fetchCover() override;

        BiliBangumi biliBangumi;
};
class TimelineItemPrviate : public TimelineItem, public QObject {
    public:
        TimelineItemPrviate() { }

        QDateTime date() override;
        int       dayOfWeek() override;
        NetResult<BangumiList> fetchBangumiList() override;

        BiliTimelineDay biliDay;

        BangumiList     bangumiListData;
        int             requestLeft = -1; //<  -1 means no request, 0 means all done
};
class TimelinePrivate    : public Timeline {
    public:
        QList<ItemPtr> items() override;

        BiliTimeline biliTimeline;
};

// BangumiPrviate
QStringList BangumiPrivate::availableSource() {
    return QStringList("bilibili");
}
QString     BangumiPrivate::description() {
    return biliBangumi.evaluate;
}
QString     BangumiPrivate::title() {
    return biliBangumi.title;
}
NetResult<QImage> BangumiPrivate::fetchCover() {
    auto result = NetResult<QImage>::Alloc();

    dataService->biliClient.fetchFile(biliBangumi.cover)
        .then([result](const Result<QByteArray> &data) mutable {
            Result<QImage> image;
            if (data) {
                image = QImage::fromData(data.value());
            }
            result.putResult(image);
    });
    return result;
}


// TimelineItem
QDateTime TimelineItemPrviate::date() {
    return biliDay.date;
}
int       TimelineItemPrviate::dayOfWeek() {
    return biliDay.dayOfWeek;
}
NetResult<BangumiList> TimelineItemPrviate::fetchBangumiList() {
    auto result = NetResult<BangumiList>::Alloc();
    requestLeft = biliDay.episodes.size();

    for (const auto &ep : biliDay.episodes) {
        dataService->biliClient.fetchBangumiByEpisodeID(ep.episodeID)
            .then(this, [this, result](const Result<BiliBangumi> &ban) mutable {
                if (ban) {
                    auto b = std::make_shared<BangumiPrivate>();
                    b->biliBangumi = ban.value();
                    bangumiListData.push_back(b);
                }
                requestLeft -= 1;

                if (requestLeft == 0) {
                    // Done
                    result.putResult(bangumiListData);
                }
        });
    }

    return result;
}

// Timeline
QList<TimelineItemPtr> TimelinePrivate::items() {
    QList<TimelineItemPtr> items;
    for (const auto &d : biliTimeline) {
        auto item = std::make_shared<TimelineItemPrviate>();
        item->biliDay = d;

        items.push_back(item);
    }
    return items;
}


// DataService
NetResultPtr<Timeline> DataServicePrivate::fetchTimeline() {
    auto result = NetResultPtr<Timeline>::Alloc();
    biliClient.fetchTimeline().then(this, [this, result](const Result<BiliTimeline> &t) mutable {
        if (!t) {
            result.putResult(std::nullopt);
            return;
        }
        auto timeline = std::make_shared<TimelinePrivate>();
        timeline->biliTimeline = t.value();

        result.putResult(timeline->as<Timeline>());
    });
    return result;
}

DataService *DataService::instance() {
    if (!dataService) {
        dataService = new DataServicePrivate(qApp);
    }
    return dataService;
}
