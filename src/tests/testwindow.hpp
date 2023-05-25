#pragma once

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QThread>
#include <functional>

#include "../log.hpp"
#include "testregister.hpp"

class Worker : public QThread {
    Q_OBJECT

public:
    Worker() = default;
    Worker(std::function<void*()> task) : task(task) {}
    void run() override;
    void setTask(std::function<void*()> task);

Q_SIGNALS:
    void finished(void *);

private:
    std::function<void*()> task;
};


class ZoodTestWindow : public QMainWindow {
    Q_OBJECT
    public:
        
        ZoodTestWindow(QWidget *parent = nullptr);
        ~ZoodTestWindow();
        QWidget* TerminatorParent();
        void RunAllTest();

    public:
        void showEvent(QShowEvent* event) override;

    public Q_SLOTS:
		void ItemClicked(QTreeWidgetItem *item, int column);
    protected:
        void keyPressEvent(QKeyEvent *) override;
    private:
        void *ui = nullptr;
		QMap<QTreeWidgetItem*, QObject*> items;
		QTreeWidgetItem* currentItem = nullptr;
    friend void ZoodLogString(const QString &text);
};