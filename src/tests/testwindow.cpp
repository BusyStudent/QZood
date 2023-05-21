#include "testwindow.hpp"
#include "ui_testwindow.h"
#include <QKeyEvent>
#include <QListWidget>
#include <QDateTime>
#include <QToolButton>

#if defined(_WIN32)
#include <Windows.h>

#define SetConsoleColor(color)                                          \
	::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), color);  \
	::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE), color);

#define CONSOLE_RED     FOREGROUND_RED
#define CONSOLE_GREEN   FOREGROUND_GREEN
#define CONSOLE_BLUE    FOREGROUND_BLUE
#define CONSOLE_DEFAULT CONSOLE_RED | CONSOLE_GREEN | CONSOLE_BLUE
#define CONSOLE_YELLOW  CONSOLE_GREEN | FOREGROUND_RED
#define CONSOLE_LIGHTBLUE  CONSOLE_GREEN | FOREGROUND_BLUE
#else

#endif


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

	auto ret = QString("[%1] %2").arg(timestring, text);

	ui->logConsoleView->addItem(ret);
	ui->logConsoleView->scrollToBottom();
	ui->statusbar->showMessage(ret, 5);
}

QWidget* ZoodTestWindow::TerminatorParent() {
	auto dui = static_cast<Ui::MainWindow*>(ui);
    
    return dui->consoleView;
}


ZoodTestWindow::ZoodTestWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
	auto dui = static_cast<Ui::MainWindow*>(ui);

	dui->setupUi(this);
	QAction *clear = new QAction();
	clear->setText("clear");

    dui->tabWidget->setTabText(0, "日志");
    dui->tabWidget->setTabText(1, "调试控制台");

	connect(clear, &QAction::triggered, dui->logConsoleView, &QListWidget::clear);
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

	// qInstallMessageHandler
	qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &, const QString &msg) {
		switch (type) {
			case QtDebugMsg :  SetConsoleColor(CONSOLE_LIGHTBLUE); printf("qDebug "); break;
			case QtWarningMsg : SetConsoleColor(CONSOLE_YELLOW); printf("qWarn "); break;
			case QtCriticalMsg : SetConsoleColor(CONSOLE_RED); printf("qCrit "); break;
			case QtFatalMsg : SetConsoleColor(CONSOLE_RED); printf("qError "); break;
			case QtInfoMsg : SetConsoleColor(CONSOLE_DEFAULT); printf("qInfo "); break;
		}
		printf("%s", msg.toLocal8Bit().data());

		SetConsoleColor(CONSOLE_DEFAULT);

		printf("\n");
	});
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

void ZoodTestWindow::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_F11) {
		if (window()->isFullScreen()) {
			window()->showNormal();
		}
		else {
			window()->showFullScreen();
		}
	}
}

ZoodTestWindow::~ZoodTestWindow() {
	delete static_cast<Ui::MainWindow*>(ui);
}