#include "../player/videocanvas.hpp"
#include "../net/bilibili.hpp"
#include "../log.hpp"
#include "testregister.hpp"
#include "ui_biliplay.h"

#include <QInputDialog>

ZOOD_TEST(Network, BiliPlay) {
    auto root = new QWidget();
    auto canvas = new VideoCanvas();
    auto player = new NekoMediaPlayer(root);
    auto bili = new BiliClient(root);
    auto audioOutput = new NekoAudioOutput(root);

    player->setAudioOutput(audioOutput);

    Ui::BiliPlay play;
    play.setupUi(root);

    play.verticalLayout->addWidget(canvas, 1);
    canvas->attachPlayer(player);

    QObject::connect(player, &NekoMediaPlayer::durationChanged, [=](qreal dur) {
        play.durationLabel->setText(QString::asprintf("%.2f", dur));
        play.progressSlider->setRange(0, dur);
    });
    QObject::connect(player, &NekoMediaPlayer::positionChanged, [=](qreal pos) {
        play.positionLabel->setText(QString::asprintf("%.2f", pos));

        if (play.progressSlider->isSliderDown()) {
            // Pressed
            return;
        }
        play.progressSlider->setValue(pos);
    });
    QObject::connect(play.danLimitButton, &QPushButton::clicked, [=]() {
        auto v = QInputDialog::getDouble(
            root, 
            "Input limit ratio", 
            "Ratio", canvas->danmakuTracksLimit(), 
            0.0, 
            1.0, 
            1, 
            nullptr, 
            {},
            0.1
        );
        canvas->setDanmakuTracksLimit(v);
    });
    QObject::connect(play.pauseButton, &QPushButton::clicked, [=]() {
        switch (player->playbackState()) {
            case NekoMediaPlayer::PlayingState : {
                // Current is playing
                player->pause();
                break;
            }
            case NekoMediaPlayer::PausedState : {
                player->play();
                break;
            }
        }
    });
    QObject::connect(play.progressSlider, &QSlider::sliderMoved, [=](int pos) {
        player->setPosition(pos);
        canvas->setDanmakuPosition(pos);
    });
    QObject::connect(player, &NekoMediaPlayer::playbackStateChanged, [=](NekoMediaPlayer::PlaybackState s) {
        switch (s) {
            case NekoMediaPlayer::PlayingState : {
                // Current is playing
                play.pauseButton->setEnabled(true);
                play.progressSlider->setEnabled(player->isSeekable());
                play.pauseButton->setText("Pause");
                break;
            }
            case NekoMediaPlayer::PausedState : {
                play.pauseButton->setEnabled(true);
                play.progressSlider->setEnabled(player->isSeekable());
                play.pauseButton->setText("Resume");
                break;
            }
            case NekoMediaPlayer::StoppedState : {
                play.pauseButton->setDisabled(true);
                play.progressSlider->setDisabled(true);
                break;
            }
        }
    });
    QObject::connect(play.muteButton, &QPushButton::clicked, [=]() { 
        audioOutput->setMuted(!audioOutput->isMuted());
        if (audioOutput->isMuted()) {
            play.muteButton->setText("Clear Mute");
        }
        else {
            play.muteButton->setText("Mute");
        }
    });
    auto playByBVID = [=](const QString &bvid) {
        bili->convertToCid(bvid).then([=](const Result<QString> &cid) mutable {
            if (!cid) {
                return;
            }
            bili->fetchDanmaku(cid.value()).then([=](const Result<DanmakuList> &dan) mutable {
                if (dan) {
                    ZOOD_QLOG("Video Danmaku ready", "");
                    canvas->setDanmakuList(dan.value());
                }
            });
            bili->fetchVideoSource(cid.value(), bvid).then([=](const Result<BiliVideoSource> &src) mutable {
                if (src) {
                    player->setHttpReferer("https://www.bilibili.com");
                    player->setHttpUseragent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537");
                    player->setOption("multiple_requests", "1");
                    player->setSource(src.value().urls.first());
                    player->play();
                }
            });
        });
    };
    QObject::connect(play.loadButton, &QPushButton::clicked, [=]() mutable {
        auto url = QInputDialog::getText(root, "Enter a URL", "URL");
        if (url.isNull()) {
            return;
        }
        auto result = bili->parseUrl(url);
        if (!result.episodeID.isNull()) {
            // DanmakuList empty;
            player->stop();
            canvas->setDanmakuList(DanmakuList());
            bili->fetchBangumiByEpisodeID(result.episodeID).then([=](const Result<BiliBangumi>& b) {
                if (!b) {
                    return;
                }
                for (auto &ep : b.value().episodes) {
                    if (result.episodeID == ep.id) {
                        playByBVID(ep.bvid);
                    }
                }
            });
            return;
        }
        if (result.bvid.isNull()) {
        }
        // DanmakuList empty;
        player->stop();
        canvas->setDanmakuList(DanmakuList());

        // Begin fecth
        playByBVID(result.bvid);
    });
    QObject::connect(player, &NekoMediaPlayer::bufferProgressChanged, [=](float n) {
        ZOOD_QLOG("MediaPlayer buffer progress changed to %1", QString::number(n));
    });
    QObject::connect(player, &NekoMediaPlayer::seekableChanged, [=](bool v) {
        play.progressSlider->setEnabled(v);
    });

    return root;
}