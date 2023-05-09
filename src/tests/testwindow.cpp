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

	ui->logConsoleView->addItem(QString("[%1] %2").arg(timestring, text));
}

ZoodTestWindow::ZoodTestWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
	auto dui = static_cast<Ui::MainWindow*>(ui);

	dui->setupUi(this);
	QAction *clear = new QAction();
	clear->setText("clear");
	connect(clear, &QAction::trigger, dui->logConsoleView, &QListWidget::clear);
	dui->logConsoleView->addAction(clear);
		
    // Register test
	dui->testList->setHeaderLabel("测试项");
	connect(dui->testList, &QTreeWidget::itemClicked, this, &ZoodTestWindow::ItemClicked);
	for (const auto &[name, fn] : GetList()) {
		auto wi = fn();
		if (!wi) {
			continue;
		}
		QTreeWidgetItem *item = new QTreeWidgetItem(dui->testList);
		item->setText(0,name);
		items.insert(item, wi);
		dui->testUiContainer->addWidget(wi);
	}

	test_window = this;
}

void ZoodTestWindow::ItemClicked(QTreeWidgetItem *item, int column) {
	if (currentItem == item) {
		return;
	}
	auto dui = static_cast<Ui::MainWindow*>(ui);
  
	auto wi = items.find(item);
	if (wi != items.end()) {
		dui->testUiContainer->setCurrentWidget(*wi);
	}
	currentItem = item;
}

ZoodTestWindow::~ZoodTestWindow() {
		delete static_cast<Ui::MainWindow*>(ui);
}