#include "../player/videocanvas.hpp"
#include "../log.hpp"
#include "testwindow.hpp"
#include <QVideoWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QNetworkRequest>
#include <QInputDialog>
#include <QFile>

#include "ui_videocanvastest.h"

ZOOD_TEST(VideoCanvas) {
    auto root = new QWidget;
    auto vcanvas = new VideoCanvas;
    // auto vwidget = new QVideoWidget;

    auto player = new QMediaPlayer(root);
    Ui::VideoCanvasTest form;

    form.setupUi(root);
    form.verticalLayout->addWidget(vcanvas, 1);
    // form.verticalLayout->addWidget(vwidget, 1);
    


    vcanvas->attachPlayer(player);
    // player->setVideoOutput(vwidget);

    QObject::connect(form.loadButton, &QPushButton::clicked, [=]() {
        auto result = QFileDialog::getOpenFileUrl(root, "Select a url to display");
        qDebug() << result;
        if (!result.isEmpty()) {
            player->setMedia(result);
            player->play();
        }
    });
    QObject::connect(form.puaseButton, &QPushButton::clicked, [=]() {
        switch (player->state()) {
            case QMediaPlayer::PlayingState : {
                // Current is playing
                player->pause();
                break;
            }
            case QMediaPlayer::PausedState : {
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
    QObject::connect(player, &QMediaPlayer::stateChanged, [=](QMediaPlayer::State s) {
        switch (s) {
            case QMediaPlayer::PlayingState : {
                // Current is playing
                form.puaseButton->setEnabled(true);
                form.progressSlider->setEnabled(true);
                form.puaseButton->setText("Pause");
                break;
            }
            case QMediaPlayer::PausedState : {
                form.puaseButton->setEnabled(true);
                form.progressSlider->setEnabled(true);
                form.puaseButton->setText("Resume");
                break;
            }
            case QMediaPlayer::StoppedState : {
                form.puaseButton->setDisabled(true);
                form.progressSlider->setDisabled(true);
                break;
            }
        }
    });
    QObject::connect(form.urlButton, &QPushButton::clicked, [=]() {
        auto url = QInputDialog::getText(root, "Enter a URL", "URL");
        if (url.isEmpty()) {
            return;
        }
        url = url.trimmed(); // Remove whitespace from beginning and end of text box.
        
        QNetworkRequest req;
        req.setUrl(url);
        req.setRawHeader("Referer", "https://www.bilibili.com");

        player->setMedia(req);
        player->play();
    });
    QObject::connect(form.progressSlider, &QSlider::sliderMoved, [=](int pos) {
        player->setPosition(pos);
    });
    QObject::connect(player, &QMediaPlayer::positionChanged, [=](qint64 position) {
        form.progressSlider->setRange(0, player->duration());
        form.progressSlider->setValue(position);
    });
    QObject::connect(player, &QMediaPlayer::positionChanged, [=](qint64 position) {
        form.progressSlider->setRange(0, player->duration());
        form.progressSlider->setValue(position);
    });
    QObject::connect(player, static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error), [=](QMediaPlayer::Error error) {
        qDebug() << error;

        ZOOD_QLOG("Failed to play video %1", player->errorString());
    });

    return root;
}