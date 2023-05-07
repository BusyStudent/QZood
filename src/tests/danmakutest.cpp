#include <QMessageBox>
#include <QDebug>
#include "testwindow.hpp"
#include "ui_danmakutest.h"

#include "../danmaku.hpp"

ZOOD_TEST(DanmakuParse) {
    auto root = new QWidget;

    Ui::Form form;
    form.setupUi(root);

    QObject::connect(form.parseButton, &QPushButton::clicked, [=]() {
        auto text = form.inputEdit->toPlainText();

        qDebug() << text;

        auto result = ParseDanmaku(text);

        form.outputEdit->clear();

        if (!result) {
            QMessageBox::critical(root, "Error", "Failed to Parse");
            return;
        }

        QString ret;
        for (const auto &item : result.value()) {
            ret += QString("%1 %2\n").arg(item.text, QString::asprintf("%d", item.position));
        }
        form.outputEdit->setPlainText(text);

    });

    return root;
}