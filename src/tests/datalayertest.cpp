#include "testregister.hpp"
#include "../net/datalayer.hpp"
#include <QTreeWidget>
#include <QListWidget>

ZOOD_TEST(DataLayer, Timeline) {
    auto table = new QTreeWidget();
    
    DataService::instance()->fetchTimeline().then([table](const ResultPtr<Timeline> &tm) {
        if (!tm) {
            auto i = new QTreeWidgetItem(table, QStringList("failed to fetch timeline"));
            table->addTopLevelItem(i);
            return;
        }
        auto v = tm.value();

        for (const auto &each : v->items()) {
            each->fetchBangumiList().then([each, table](const Result<BangumiList> &list) {
                auto i = new QTreeWidgetItem(table, QStringList(QString::number(each->dayOfWeek())));
                table->addTopLevelItem(i);

                // i->setText(QString::number(each->dayOfWeek()));
                if (!list) {
                    // table->addItem("Failed to fetch");
                    new QTreeWidgetItem(i, QStringList("Failed to fetch"));
                    return;
                }
                for (const auto &ep : list.value()) {
                    // table->addItem(ep->title());
                    // table->addItem(ep->description());
                    auto sub = new QTreeWidgetItem(i, QStringList(ep->title()));
                    new QTreeWidgetItem(sub, QStringList(ep->description()));
                    for (auto v : ep->availableSource()) {
                        new QTreeWidgetItem(sub, QStringList(QString("Source from %1").arg(v)));
                    }

                    ep->fetchCover().then([sub](const Result<QImage> &image) {
                        if (image) {
                            sub->setIcon(0, QPixmap::fromImage(image.value()));
                        }
                    });
                }
            });
        }
    });

    return table;
}