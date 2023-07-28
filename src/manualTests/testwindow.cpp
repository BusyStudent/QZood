#include "testwindow.hpp"
#include "ui_testwindow.h"
#include <QKeyEvent>
#include <QListWidget>
#include <QDateTime>
#include <QThreadPool>
#include <QToolButton>
#include <QMenu>

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
    setWindowFlag(Qt::Window);
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

    menu = new QMenu(dui->testList);
    dui->testList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(dui->testList, 
            &QTreeWidget::customContextMenuRequested, 
            this,[this](const QPoint &pos){
                auto dui = static_cast<Ui::MainWindow*>(ui);
                auto item = dui->testList->itemAt(pos);
                if (item != nullptr) {
                    ShowMenu(item, pos);
                }
            });
    
    // Register test
	dui->testList->setHeaderLabels(QStringList{"测试列表", "完成进度"});
    QMap<QString, QTreeWidgetItem*> tree_items;
    dui->testList->setIndentation(10);
    
    for (const auto &test_task : GetList()) {
        auto item_ = tree_items.find(test_task.module_name);
        if (item_ == tree_items.end()) {
            tree_items[test_task.module_name] = new QTreeWidgetItem(dui->testList, QStringList(test_task.module_name));
        }
        
        testModules[tree_items[test_task.module_name]].push_back(test_task.id);
        testTasks[test_task.id] = test_task;

        auto item = new QTreeWidgetItem(tree_items[test_task.module_name], QStringList(test_task.test_name));
        items.insert(item, test_task.id);
		RegisterTest(test_task, item);
	}
}

void ZoodTestWindow::RegisterTest(const TestTask &task, QTreeWidgetItem* item) {
    auto dui = static_cast<Ui::MainWindow*>(ui);

    item->setText(1, "testing");
    item->setForeground(1, QColor("#daa520"));

    if (testWidgets.find(task.id) != testWidgets.end()) {
        testWidgets[task.id]->deleteLater();
        testWidgets[task.id] = nullptr;
    }

    if (task.type == TestType::WIDGET) {
        auto wi = static_cast<QWidget*>(task.task());
        if (wi) {
            auto widget = static_cast<QWidget*>(wi);
            dui->testUiContainer->addWidget(widget);
            testWidgets[task.id] = widget;
        }
    } else if (task.type == TestType::WIDGET_W) {
        auto wi = static_cast<QWidget*>(task.task());
        if (wi) {
            auto widget = static_cast<QWidget*>(wi);
            widget->setParent(this);
            widget->setWindowFlag(Qt::Window);
            testWidgets[task.id] = widget;
        }
    } else if (task.type == TestType::CMD) {
        QMetaObject::invokeMethod(cmdHelperObject, [item, dui, task, this] {
            task.task();
            QMetaObject::invokeMethod(test_window, [=]() {
                if (TestFlag(task.id)) {
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

void ZoodTestWindow::TestCompleter(QTreeWidgetItem* item) {
    auto id = items[item];
    auto task = testTasks[id];
    if (TestFlag(task.id)) {
        item->setText(1, "finished");
        item->setForeground(1, QColor("#008000"));
    } else {
        item->setText(1, "falied");
        item->setForeground(1, QColor("#b22222"));
    }
    auto obj = testWidgets[id];
    if (obj != nullptr) {
        obj->deleteLater();
    }
    testWidgets[id] = nullptr;
}

void ZoodTestWindow::ItemClicked(QTreeWidgetItem *item, int column) {
	if (currentItem == item) {
		return;
	}
	auto dui = static_cast<Ui::MainWindow*>(ui);
  
	auto id = items.find(item);
	if (id != items.end()) {
        ShowTestUi(*id);
	}
	currentItem = item;
}

void ZoodTestWindow::ShowTestUi(const uint64_t id) {
	auto dui = static_cast<Ui::MainWindow*>(ui);

    auto wi = testWidgets.find(id);
    if (wi == testWidgets.end() || (*wi) == nullptr) {
        return;
    }
    auto &task = testTasks[id];
    if (task.type == TestType::WIDGET && (*wi)->isWidgetType()) {
        dui->testUiContainer->setCurrentWidget(static_cast<QWidget*>(*wi));
    } else if ((*wi)->isWidgetType()){
        static_cast<QWidget*>(*wi)->show();
    }
}

void ZoodTestWindow::RestartTest(QTreeWidgetItem* item) {
    if (testModules.find(item) != testModules.end()) {
        for (const auto id : testModules[item]) {
            RegisterTest(testTasks[id], item);
        }
        return;
    }
    if (items.find(item) == items.end()) {
        return;
    }
    auto id = items[item];
    RegisterTest(testTasks[id], item);
}

void ZoodTestWindow::ShowTestUi(QTreeWidgetItem* item) {
    auto dui = static_cast<Ui::MainWindow*>(ui);
    dui->testList->setCurrentItem(item);

    if (testModules.find(item) != testModules.end()) {
        for (const auto id : testModules[item]) {
            ShowTestUi(id);
        }
        return;
    }
    if (items.find(item) == items.end()) {
        return;
    }
    auto id = items[item];
    ShowTestUi(id);
}

void ZoodTestWindow::HideTestUi(QTreeWidgetItem* item) {
    if (testModules.find(item) != testModules.end()) {
        for (const auto id : testModules[item]) {
            HideTestUi(id);
        }
        return;
    }
    if (items.find(item) == items.end()) {
        return;
    }
    auto id = items[item];
    HideTestUi(id);
}

void ZoodTestWindow::HideTestUi(const uint64_t id) {
    auto dui = static_cast<Ui::MainWindow*>(ui);

    auto wi = testWidgets.find(id);
    if (wi == testWidgets.end() || nullptr == (*wi)) {
        return;
    }
    auto &task = testTasks[id];
    if (task.type == TestType::WIDGET_W && (*wi)->isWidgetType()){
        static_cast<QWidget*>(*wi)->close();
    }
}

void ZoodTestWindow::ShowMenu(QTreeWidgetItem* item, const QPoint& pos) {
    auto dui = static_cast<Ui::MainWindow*>(ui);
    QList<QAction*> actionList;
    
    if (testModules.find(item) != testModules.end()) {
        actionList.push_back(menu->addAction("重新测试", [this, item, dui](){
            RestartTest(item);
            dui->testList->setCurrentItem(item);
        }));
    } else {
        auto id = items.find(item);
        if (id != items.end()) {
            if (testTasks[*id].type == TestType::WIDGET_W && nullptr != testWidgets[*id]) {
                actionList.push_back(menu->addAction("显示测试界面", [this, item](){
                    ShowTestUi(item);
                }));
                actionList.push_back(menu->addAction("隐藏测试界面", [this, item](){
                    HideTestUi(item);
                }));
                actionList.push_back(menu->addSeparator());
            }
            actionList.push_back(menu->addAction("重新测试", [this, item](){
                    RestartTest(item);
            }));
            if (testTasks[*id].type != TestType::CMD && nullptr != testWidgets[*id]) {
                actionList.push_back(menu->addAction("界面测试完成", [this, item](){
                    TestCompleter(item);
                }));
            }
        }
    }

    menu->exec(dui->testList->mapToGlobal(pos));

    for (auto action : actionList) {
        menu->removeAction(action);
        action->deleteLater();
    }
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