#include "../player/videocanvas.hpp"
#include "../log.hpp"
#include "../net/client.hpp"
#include "testregister.hpp"
#include <QMessageBox>
#include <QFileDialog>
#include <QNetworkRequest>
#include <QInputDialog>
#include <QMetaEnum>
#include <QFile>

#include "ui_videocanvastest.h"


ZOOD_TEST(Player, VideoCanvas) {
    qDebug() << "Supported video formats" << NekoMediaPlayer::supportedMediaTypes();
    qDebug() << "Supported protocols" << NekoMediaPlayer::supportedProtocols();

    auto root = new QWidget;
    auto vcanvas = new VideoCanvas;
    auto audio = new NekoAudioOutput(root);
    // auto vwidget = new QVideoWidget;

    auto player = new NekoMediaPlayer(root);
    Ui::VideoCanvasTest form;

    form.setupUi(root);
    form.verticalLayout->addWidget(vcanvas, 1);
    // form.verticalLayout->addWidget(vwidget, 1);
    


    vcanvas->attachPlayer(player);
    player->setAudioOutput(audio);
    // player->setVideoOutput(vwidget);

    QObject::connect(form.loadButton, &QPushButton::clicked, [=]() {
        auto result = QFileDialog::getOpenFileUrl(root, "Select a url to display");
        qDebug() << result;
        if (!result.isEmpty()) {
            player->setSource(result);
            player->play();
        }
    });
    QObject::connect(form.puaseButton, &QPushButton::clicked, [=]() {
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
    QObject::connect(form.danmakuButton, &QPushButton::clicked, [=]() {
        auto result = QFileDialog::getOpenFileUrl(root, "Select a danmaku to display");
        if (!result.isEmpty()) {
            QFile file(result.toLocalFile());
            if (file.open(QFile::ReadOnly)) {
                auto danmaku = ParseDanmaku(file.readAll());
                if (danmaku) {
                    vcanvas->setDanmakuList(danmaku.value());
                }
                else {
                    QMessageBox::critical(root, "Error", "Failed to Parse");
                }
            }
        }
    });
    QObject::connect(player, &NekoMediaPlayer::playbackStateChanged, [=](NekoMediaPlayer::PlaybackState s) {
        switch (s) {
            case NekoMediaPlayer::PlayingState : {
                // Current is playing
                form.puaseButton->setEnabled(true);
                form.progressSlider->setEnabled(player->isSeekable());
                form.puaseButton->setText("Pause");
                break;
            }
            case NekoMediaPlayer::PausedState : {
                form.puaseButton->setEnabled(true);
                form.progressSlider->setEnabled(player->isSeekable());
                form.puaseButton->setText("Resume");
                break;
            }
            case NekoMediaPlayer::StoppedState : {
                form.puaseButton->setDisabled(true);
                form.progressSlider->setDisabled(true);
                break;
            }
        ZOOD_QLOG("MediaPlayer state changed to %1", QMetaEnum::fromType<NekoMediaPlayer::PlaybackState>().valueToKey(s));
        }
    });
    QObject::connect(player, &NekoMediaPlayer::mediaStatusChanged, [=](NekoMediaPlayer::MediaStatus s) {
        ZOOD_QLOG("MediaPlayer status changed to %1", QMetaEnum::fromType<NekoMediaPlayer::MediaStatus>().valueToKey(s));
    });
    QObject::connect(player, &NekoMediaPlayer::bufferProgressChanged, [=](float n) {
        ZOOD_QLOG("MediaPlayer status changed to %1", n);
    });
    QObject::connect(player, &NekoMediaPlayer::seekableChanged, [=](bool v) {
        form.progressSlider->setEnabled(v);
    });
    QObject::connect(form.urlButton, &QPushButton::clicked, [=]() {
        auto url = QInputDialog::getText(root, "Enter a URL", "URL");
        if (url.isEmpty()) {
            return;
        }
        url = url.trimmed(); // Remove whitespace from beginning and end of text box.

        player->setHttpReferer("https://www.bilibili.com");
        player->setHttpUseragent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537");
        player->setSource(url);
        player->play();
    });
    QObject::connect(form.progressSlider, &QSlider::sliderMoved, [=](int pos) {
        player->setPosition(pos);
    });
    QObject::connect(player, &NekoMediaPlayer::positionChanged, [=](qint64 position) {
        if (form.progressSlider->isSliderDown()) {
            return;
        }
        form.progressSlider->setRange(0, player->duration());
        form.progressSlider->setValue(position);
    });

    QObject::connect(player, &NekoMediaPlayer::errorOccurred, [=](NekoMediaPlayer::Error error, const QString &errorString) {
        qDebug() << error;

        ZOOD_QLOG("Failed to play video %1", errorString);
    });

    return root;
}
