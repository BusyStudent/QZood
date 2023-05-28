#include "testwindow.hpp"
#include "ui_testwindow.h"
#include <QKeyEvent>
#include <QListWidget>
#include <QDateTime>
#include <QThreadPool>
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

struct TestTask {
    TestTask() {};
    TestTask(const uint64_t id,const QString &module_name, const QString &test_name, const TestType type,const std::function<void*()> &task) : 
        id(id), module_name(module_name),test_name(test_name),type(type),task(task) {}
    uint64_t id;
    QString module_name;
    QString test_name;
    TestType type;
    std::function<void*()> task;
};

static QList<TestTask> &GetList() {
	static QList<TestTask> lt;
	return lt;
}
QMap<int, bool>& TestFlags() {
    static QMap<int, bool> flags;
    return flags;
}
static ZoodTestWindow *test_window = nullptr;

void ZoodRegisterTest(const QString &module_name, const QString &name,const TestType type, const std::function<void*(const int id)> &task) {
    static uint64_t id = 0;
    ++id;
	GetList().push_back(TestTask(id, module_name, name, type, std::bind(task, id)));
}
void ZoodLogString(const QString &text) {
	if (!test_window) {
		return;
	}
	auto date = QDateTime::currentDateTime();
	auto timestring = date.toString("dd.MM.yyyy hh:mm:ss");

	auto ret = QString("[%1] %2").arg(timestring, text);

    QMetaObject::invokeMethod(
        test_window,
        [=]() {
            auto ui = static_cast<Ui::MainWindow*>(test_window->ui);
            ui->logConsoleView->addItem(ret);
            ui->logConsoleView->scrollToBottom();
            ui->statusbar->showMessage(ret, 5);
        },
        Qt::QueuedConnection
    );
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
		

	connect(dui->testList, &QTreeWidget::itemClicked, this, &ZoodTestWindow::ItemClicked);
	

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

    // Start Test Thread
    cmdWorkerThread = new QThread();
    cmdWorkerThread->setObjectName("cmdWorkerThread");
    cmdWorkerThread->start();

    cmdHelperObject = new QObject();
    cmdHelperObject->moveToThread(cmdWorkerThread);
}

void ZoodTestWindow::showEvent(QShowEvent* event) {
    static bool flag = (RunAllTest(), true);
    QMainWindow::showEvent(event);
}

void ZoodTestWindow::RunAllTest() {
	auto dui = static_cast<Ui::MainWindow*>(ui);
    
    // Register test
	dui->testList->setHeaderLabels(QStringList{"测试列表", "完成进度"});
    QMap<QString, QTreeWidgetItem*> tree_items;
    dui->testList->setIndentation(10);
    for (const auto &test_task : GetList()) {
        auto item_ = tree_items.find(test_task.module_name);
        if (item_ == tree_items.end()) {
            tree_items[test_task.module_name] = new QTreeWidgetItem(dui->testList, QStringList(test_task.module_name));
        }
        auto item = new QTreeWidgetItem(tree_items[test_task.module_name], QStringList(test_task.test_name));
        item->setText(1, "testing");
        item->setForeground(1, QColor("#daa520"));
		if (test_task.type == TestType::WIDGET) {
            auto wi = static_cast<QWidget*>(test_task.task());
            if (wi) {
                auto widget = static_cast<QWidget*>(wi);
                dui->testUiContainer->addWidget(widget);
                items.insert(item, widget);
            }
            if (TestFlag(test_task.id) && wi) {
                item->setText(1, "finished");
                item->setForeground(1, QColor("#008000"));
            } else {
                item->setText(1, "failed");
                item->setForeground(1, QColor("#b22222"));
            }
        } else if (test_task.type == TestType::WIDGET_W) {
            auto wi = static_cast<QWidget*>(test_task.task());
            if (wi) {
                auto widget = static_cast<QWidget*>(wi);
                widget->show();
                widget->setAttribute(Qt::WA_DeleteOnClose);
                connect(this, &QWidget::destroyed, widget, [widget](){
                    widget->close();
                });
                connect(widget, &QWidget::destroyed, this, [item, test_task](){
                    if (TestFlag(test_task.id)) {
                        item->setText(1, "finished");
                        item->setForeground(1, QColor("#008000"));
                    } else {
                        item->setText(1, "failed");
                        item->setForeground(1, QColor("#b22222"));
                    }
                });
            }
        } else if (test_task.type == TestType::CMD) {
            QMetaObject::invokeMethod(cmdHelperObject, [item, dui, test_task, this] {
                test_task.task();
                QMetaObject::invokeMethod(test_window, [=]() {
                    if (TestFlag(test_task.id)) {
                        item->setText(1, "finished");
                        item->setForeground(1, QColor("#008000"));
                    } else {
                        item->setText(1, "falied");
                        item->setForeground(1, QColor("#b22222"));
                    }
                });
            });
        }
	}
}

void ZoodTestWindow::ItemClicked(QTreeWidgetItem *item, int column) {
	if (currentItem == item) {
		return;
	}
	auto dui = static_cast<Ui::MainWindow*>(ui);
  
	auto wi = items.find(item);
	if (wi != items.end()) {
        if ((*wi) && (*wi)->isWidgetType()) {
    		dui->testUiContainer->setCurrentWidget(static_cast<QWidget*>(*wi));
        }
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
    cmdWorkerThread->exit();
    cmdWorkerThread->wait();
    delete cmdWorkerThread;
    delete cmdHelperObject;
	delete static_cast<Ui::MainWindow*>(ui);
}