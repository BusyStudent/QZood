#include <QMessageBox>
#include "testwindow.hpp"
#include "ui_bilitest.h"

#include "../net/bilibili.hpp"

ZOOD_TEST(Bilibili) {
    struct Date : public QObject {
        using QObject::QObject;
    };

    auto root = new QWidget;
    auto bili = new BiliClient(root);
    auto date = new Date(root);

    Ui::BiliTest ui;
    ui.setupUi(root);

    QObject::connect(ui.bvidButton, &QPushButton::clicked, [=]() {
        auto result = bili->parseBvid(ui.urlInEdit->text());
        if (result) {
            ui.bvidOutEdit->setText(result.value());
        }
        else {
            ui.bvidOutEdit->clear();
        }
    });
    QObject::connect(ui.cidButton, &QPushButton::clicked, [=]() {
        bili->convertToCid(ui.bvidInEdit->text()).then([=](const Result<QString> &cid) {
            if (!cid) {
                QMessageBox::critical(root, "Error", "Failed to Get CID");
                return;
            }
            ui.cidOutEdit->setText(cid.value());
        });
    });
    QObject::connect(ui.danButton, &QPushButton::clicked, [=]() {
        bili->fetchDanmaku(ui.danEdit->text()).then([=](const Result<DanmakuList> &dan) {
            if (!dan) {
                QMessageBox::critical(root, "Error", "Failed to Get Danmaku");
                return;
            }
            QString ret;

            ret += QString("Num of danmaku %1\n").arg(QString::number(dan.value().size()));

            for (auto &item : dan.value()) {
                ret += QString("%1 %2 %3\n").arg(QString::asprintf("Pos : %lf Size : %d", item.position, item.size), item.text, QString::asprintf("%d", item.position));
            }

            ui.bigOutEdit->setPlainText(ret);
        });
    });
    // QObject::connect(bili, &BiliClient::videoCidReady, [=](uint64_t id, const Result<QString> &cid) {
    //     if (!cid) {
    //         QMessageBox::critical(root, "Error", "Failed to Get CID");
    //         return;
    //     }
    //     ui.cidOutEdit->setText(cid.value());
    // });
    QObject::connect(ui.videoButton, &QPushButton::clicked, [=]() {
        bili->fetchVideoSource(ui.urlCidEdit->text(), ui.urlBvidEdit->text())
            .then([=](const Result<BiliVideoSource> &source) {
            if (!source) {
                QMessageBox::critical(root, "Error", "Failed to Get Danmaku");
                return;
            }

            QString ret;
            for (auto url : source.value().urls) {
                ret += QString("Source %1\n").arg(url);
            }

            ui.bigOutEdit->setPlainText(ret);
        });
    });


    return root;
}
