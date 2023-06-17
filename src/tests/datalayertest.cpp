#include "testregister.hpp"
#include "../net/datalayer.hpp"
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>

#include "ui_datalayertest.h"

ZOOD_TEST(DataLayer, Timeline) {
    InitializeVideoInterface();
    Ui::DataLayerTest ui;
    auto container = new QTabWidget();

    ui.setupUi(container);
    auto table = ui.tlTreeWidget;
    auto load = ui.tlPushButton;

    auto currentInterface = [](QComboBox *cbbox) -> VideoInterface * {
        for (auto i : GetVideoInterfaceList()) {
            if (i->name() == cbbox->currentText()) {
                return i;
            }
        }
        return nullptr;
    };
    auto prepareComboBox = [](QComboBox *cbbox) {
        for (auto i : GetVideoInterfaceList()) {
            cbbox->addItem(i->name());
        }
        cbbox->setCurrentIndex(0);
    };


    load->setText("Load timeline from DataLayer");
    prepareComboBox(ui.tlComboBox);
    prepareComboBox(ui.searchComboBox);

    QObject::connect(load, &QPushButton::clicked, [=]() {
        currentInterface(ui.tlComboBox)->fetchTimeline().then([table](const Result<Timeline> &tm) {
            if (!tm) {
                auto i = new QTreeWidgetItem(table, QStringList("failed to fetch timeline"));
                table->addTopLevelItem(i);
                return;
            }

            for (const auto &each : tm.value()) {
                each->fetchBangumiList().then([each, table](const Result<BangumiList> &list) {
                    auto i = new QTreeWidgetItem(table, QStringList(each->date().toString("yyyy.MM.dd")));
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
                        sub->setText(1,  ep->description());
                        ep->fetchCover().then([sub](const Result<QImage> &image) {
                            if (image) {
                                sub->setIcon(0, QPixmap::fromImage(image.value()));
                            }
                        });
                    }
                });
            }
        });
    });
    QObject::connect(ui.searchButton, &QPushButton::clicked, [=](){
        ui.searchListWidget->clear();
        currentInterface(ui.searchComboBox)->searchBangumi(ui.searchInputEdit->text()).then(
            [=](const Result<BangumiList> &bangumi) {
                if (!bangumi) {
                    return;
                }
                ui.searchListWidget->setIconSize(QSize(160, 200));
                for (auto &item : bangumi.value()) {
                    auto vt = new QListWidgetItem(item->title());
                    ui.searchListWidget->addItem(vt);
                    vt->setData(Qt::UserRole, QVariant::fromValue(item));

                    item->fetchCover().then(ui.searchListWidget, [=](const Result<QImage> &image) {
                        if (image) {
                            vt->setIcon(QPixmap::fromImage(image.value()));
                        }
                    });
                }
            }
        );
    });
    QObject::connect(ui.searchListWidget, &QListWidget::itemDoubleClicked, [=](QListWidgetItem *item) {
        auto ptr = item->data(Qt::UserRole).value<BangumiPtr>();
        if (!ptr) {
            return;
        }
        ptr->fetchEpisodes().then([=](const Result<EpisodeList> &eps) mutable {
            if (!eps) {
                QMessageBox::critical(container, "Error", "Failed to fetch episodes");
                return;
            }
            QStringList names;
            for (const auto &e : eps.value()) {
                names.push_back(e->title());
            }
            auto ret = QInputDialog::getItem(container, "Select a episode", "Select", names);
            for (const auto &e : eps.value()) {
                if (e->title() == ret) {
                    e->fetchVideo(e->recommendedSource()).then([=](const Result<QString> &url) {
                        if (!url) {
                            QMessageBox::critical(container, "Error", "Failed to get video url");
                            return;
                        }
                        QInputDialog::getText(container, "Get your url here", "Url", QLineEdit::Normal, url.value());
                    });
                    return;
                }
            }
        });
    });

    return container;
}