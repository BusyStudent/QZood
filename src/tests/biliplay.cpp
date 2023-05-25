#include "../player/videocanvas.hpp"
#include "../net/bilibili.hpp"
#include "../log.hpp"
#include "testwindow.hpp"
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
        play.durationLabel->setText(QString::number(dur));
        play.progressSlider->setRange(0, dur);
    });
    QObject::connect(player, &NekoMediaPlayer::positionChanged, [=](qreal pos) {
        play.positionLabel->setText(QString::number(pos));
        play.progressSlider->setValue(pos);
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
    QObject::connect(play.loadButton, &QPushButton::clicked, [=]() mutable {
        auto url = QInputDialog::getText(root, "Enter a URL", "URL");
        if (url.isNull()) {
            return;
        }
        auto result = bili->parseUrl(url);
        if (result.bvid.isNull()) {
            return;
        }
        // DanmakuList empty;
        player->stop();
        canvas->setDanmakuList(DanmakuList());

        // Begin fecth
        bili->convertToCid(result.bvid).then([=](const Result<QString> &cid) mutable {
            if (!cid) {
                return;
            }
            bili->fetchDanmaku(cid.value()).then([=](const Result<DanmakuList> &dan) mutable {
                if (dan) {
                    ZOOD_QLOG("Video Danmaku ready", "");
                    canvas->setDanmakuList(dan.value());
                }
            });
            bili->fetchVideoSource(cid.value(), result.bvid).then([=](const Result<BiliVideoSource> &src) mutable {
                if (src) {
                    player->setHttpReferer("https://www.bilibili.com");
                    player->setHttpUseragent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537");
                    player->setSource(src.value().urls.first());
                    player->play();
                }
            });
        });
    });
    QObject::connect(player, &NekoMediaPlayer::seekableChanged, [=](bool v) {
        play.progressSlider->setEnabled(v);
    });

    return root;
}