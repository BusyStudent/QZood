#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include "testregister.hpp"
#include "ui_danmakutest.h"

#include "../common/danmaku.hpp"

ZOOD_TEST(Network, DanmakuParse) {
    auto root = new QWidget;
    
    Ui::Form form;
    form.setupUi(root);

    QObject::connect(form.parseButton, &QPushButton::clicked, [=]() {
        auto text = form.inputEdit->toPlainText();
        if (text.isEmpty()) {
            return;
        }

        qDebug() << text;

        auto result = ParseDanmaku(text);

        form.outputEdit->clear();

        if (!result) {
            QMessageBox::critical(root, "Error", "Failed to Parse");
            return;
        }

        QString ret;
        for (const auto &item : result.value()) {
            ret += QString("%1 %2 %3\n").arg(QString::asprintf("Pos : %lf Size : %d", item.position, item.size), item.text, QString::asprintf("%d", item.position));
        }
        form.outputEdit->setPlainText(ret);

    });
    QObject::connect(form.openButton, &QPushButton::clicked, [=]() {
        auto url = QFileDialog::getOpenFileUrl(root);
        if (!url.isEmpty()) {
            QFile file(url.toLocalFile());
            if (file.open(QFile::ReadOnly)) {
                form.inputEdit->setPlainText(file.readAll()); // read all text to text edit field.
            }
        }
    });

    return root;
}