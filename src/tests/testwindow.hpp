#pragma once

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QThread>
#include <functional>

#include "../log.hpp"
#include "testregister.hpp"

struct TestTask;

class ZoodTestWindow : public QMainWindow {
    Q_OBJECT
    public:
        
        ZoodTestWindow(QWidget *parent = nullptr);
        ~ZoodTestWindow();
        QWidget* TerminatorParent();
        void RunAllTest();
        void RegisterTest(const TestTask &task, QTreeWidgetItem* item);

    public:
        void showEvent(QShowEvent* event) override;

    public Q_SLOTS:
		void ItemClicked(QTreeWidgetItem *item, int column);
        void RestartTest(QTreeWidgetItem* item);
        void ShowTestUi(QTreeWidgetItem* item);
        void ShowTestUi(const uint64_t id);
        void HideTestUi(QTreeWidgetItem* item);
        void HideTestUi(const uint64_t id);
        void TestCompleter(QTreeWidgetItem* item);
        void ShowMenu(QTreeWidgetItem* item, const QPoint& pos);
    
    protected:
        void keyPressEvent(QKeyEvent *) override;
    
    private:
        void *ui = nullptr;
		QMap<QTreeWidgetItem*, uint64_t>        items;
        QMap<uint64_t, QObject*>                testWidgets;
        QMap<uint64_t, TestTask>                testTasks;
        QMap<QTreeWidgetItem*, QList<uint64_t>> testModules;
		QTreeWidgetItem*                        currentItem = nullptr;
        QThread        *                        cmdWorkerThread = nullptr;
        QObject        *                        cmdHelperObject = nullptr;
        QMenu          *                        menu = nullptr;
    friend void ZoodLogString(const QString &text);
};