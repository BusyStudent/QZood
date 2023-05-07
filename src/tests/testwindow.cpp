#include "testwindow.hpp"
#include "ui_testwindow.h"
#include <QListWidget>
#include <QDateTime>

static QList<QPair<QString, std::function<QWidget*()>>> &GetList() {
    static QList<QPair<QString, std::function<QWidget*()>>> lt;
    return lt;
}

static ZoodTestWindow *test_window = nullptr;

void ZoodRegisterTest(const QString &name, const std::function<QWidget*()> &create) {
    GetList().push_back(qMakePair(name, create));
}
void ZoodLogString(const QString &text) {
    if (!test_window) {
        return;
    }
    auto ui = static_cast<Ui::MainWindow*>(test_window->ui);
    auto date = QDateTime::currentDateTime();
    auto timestring = date.toString("dd.MM.yyyy hh:mm:ss");

    ui->logWidget->addItem(QString("[%1] %2").arg(timestring, text));
}

ZoodTestWindow::ZoodTestWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    auto dui = static_cast<Ui::MainWindow*>(ui);

    dui->setupUi(this);

    // Register test
    for (const auto &[name, fn] : GetList()) {
        auto wi = fn();
        if (!wi) {
            continue;
        }
        dui->tabWidget->addTab(wi, name);
    }

    connect(dui->clearButton, &QPushButton::clicked, dui->logWidget, &QListWidget::clear);

    test_window = this;
}
ZoodTestWindow::~ZoodTestWindow() {
    delete static_cast<Ui::MainWindow*>(ui);
}