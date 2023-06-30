#include "testregister.hpp"
#include "../net/datalayer.hpp"
#include "../player/videocanvas.hpp"
#include <QVBoxLayout>
#include <QTreeWidget>
#include <QListWidget>
#include <QPushButton>
#include <QMessageBox>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>

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
        return DataService::instance();
    };
    auto prepareComboBox = [](QComboBox *cbbox) {
        for (auto i : GetVideoInterfaceList()) {
            cbbox->addItem(i->name());
        }
        cbbox->addItem(DataService::instance()->name());
        cbbox->setCurrentIndex(0);
    };

    auto mediaPlayer = new NekoMediaPlayer(container);
    auto audioOutput = new NekoAudioOutput(container);

    auto hlayout = new QHBoxLayout(ui.canvasWidget);
    auto vcanvas = new VideoCanvas();

    mediaPlayer->setAudioOutput(audioOutput);
    vcanvas->attachPlayer(mediaPlayer);
    
    hlayout->addWidget(vcanvas);
    auto addPlayPage = [=](const EpisodeList &eps) {
        ui.episodeWidget->clear();
        for (const auto &e : eps) {
            auto item = new QListWidgetItem(e->indexTitle() + " " + e->longTitle());
            item->setData(QListWidgetItem::UserType, QVariant::fromValue(e));
            ui.episodeWidget->addItem(item);
        }
    };
    QObject::connect(ui.episodeWidget, &QListWidget::itemDoubleClicked, [=](QListWidgetItem *item) {
        auto ptr = item->data(QListWidgetItem::UserType).value<EpisodePtr>();
        if (!ptr) {
            return;
        }
        mediaPlayer->stop();

        ui.danmakusBox->clear();
        ui.danmakusBox->addItems(ptr->danmakuSourceList());

        ui.sourcesBox->clear();
        ui.sourcesBox->addItems(ptr->sourcesList());
    });
    QObject::connect(mediaPlayer, &NekoMediaPlayer::durationChanged, [=](qreal dur) {
        ui.progressSlider->setRange(0, dur * 1000);
        ui.progressSlider->setValue(0);
    });
    QObject::connect(mediaPlayer, &NekoMediaPlayer::positionChanged, [=](qreal n) {
        ui.progressSlider->setValue(n * 1000);
    });
    QObject::connect(mediaPlayer, &NekoMediaPlayer::playbackStateChanged, [=](NekoMediaPlayer::PlaybackState n) {
        switch (n) {
            case NekoMediaPlayer::PlayingState : {

            }
        }
    });
    QObject::connect(ui.playButton, &QPushButton::clicked, [=]() {
        switch (mediaPlayer->playbackState()) {
            case NekoMediaPlayer::PlayingState : {
                // Current is playing
                mediaPlayer->pause();
                break;
            }
            case NekoMediaPlayer::PausedState : {
                mediaPlayer->play();
                break;
            }
        }
    });
    QObject::connect(mediaPlayer, &NekoMediaPlayer::playbackStateChanged, [=](NekoMediaPlayer::PlaybackState s) {
        switch (s) {
            case NekoMediaPlayer::PlayingState : {
                // Current is playing
                ui.playButton->setEnabled(true);
                ui.progressSlider->setEnabled(mediaPlayer->isSeekable());
                ui.playButton->setText("Pause");
                break;
            }
            case NekoMediaPlayer::PausedState : {
                ui.playButton->setEnabled(true);
                ui.progressSlider->setEnabled(mediaPlayer->isSeekable());
                ui.playButton->setText("Resume");
                break;
            }
            case NekoMediaPlayer::StoppedState : {
                ui.playButton->setDisabled(true);
                ui.progressSlider->setDisabled(true);
                break;
            }
        }
    });
    QObject::connect(ui.progressSlider, &QSlider::sliderMoved, [=](int n) {
        mediaPlayer->setPosition(n / 1000.0);
        vcanvas->setDanmakuPosition(n / 1000.0);
    });
    QObject::connect(ui.sourcesBox, &QComboBox::currentTextChanged, [=](const QString &s) {
        auto epItem = ui.episodeWidget->currentItem();
        if (!epItem) {
            return;
        }
        auto ptr = epItem->data(QListWidgetItem::UserType).value<EpisodePtr>();
        if (!ptr) {
            return;
        }
        ptr->fetchVideo(s).then([=](const Result<QString> &playUrl) {
            mediaPlayer->stop();
            if (!playUrl) {
                QMessageBox::critical(container, "Failed to fetch video", "Failed to fetch video");
                return;
            }
            mediaPlayer->setSource(playUrl.value());
            mediaPlayer->setHttpReferer("https://www.bilibili.com");
            mediaPlayer->setHttpUseragent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537");
            mediaPlayer->setOption("multiple_requests", "1");
            mediaPlayer->play();
        });
        auto b = ui.danmakusBox->currentText();
        ptr->fetchDanmaku(b).then([=](const Result<DanmakuList> &d) {
            if (!d) {
                QMessageBox::critical(container, "Failed to fetch video", "Failed to fetch video");
                return;
            }
            vcanvas->setDanmakuList(d.value());
        });
    });


    load->setText("Load timeline from DataLayer");
    prepareComboBox(ui.tlComboBox);
    prepareComboBox(ui.searchComboBox);

    QObject::connect(load, &QPushButton::clicked, [=]() {
        currentInterface(ui.tlComboBox)->fetchTimeline().then([table](const Result<Timeline> &tm) {
            table->clear();
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
                    auto vt = new QListWidgetItem(QString("%1 Source %2").arg(item->title(), item->availableSource().join(", ")));
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
            // QStringList names;
            // for (const auto &e : eps.value()) {
            //     names.push_back(e->indexTitle());
            // }
            // auto ret = QInputDialog::getItem(container, "Select a episode", "Select", names);
            // for (const auto &e : eps.value()) {
            //     if (e->indexTitle() == ret) {
            //         QString w;
            //         auto s = e->sourcesList();
                    
            //         if (s.size() != 1) {
            //             w = QInputDialog::getItem(container, "Select a source", "Select", s);
            //         }
            //         else {
            //             w = s.first();
            //         }

            //         e->fetchVideo(w).then([=](const Result<QString> &url) {
            //             if (!url) {
            //                 QMessageBox::critical(container, "Error", "Failed to get video url");
            //                 return;
            //             }
            //             auto t = QInputDialog::getText(container, "Get your url here", "Url", QLineEdit::Normal, url.value());
            //             QApplication::clipboard()->setText(t);
            //         });
            //         return;
            //     }
            // }
            addPlayPage(eps.value());
        });
    });

    return container;
}