#pragma once

#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QThread>
#include <functional>

#include "../log.hpp"
#include "testregister.hpp"


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
        QThread        * cmdWorkerThread = nullptr;
        QObject        * cmdHelperObject = nullptr;
    friend void ZoodLogString(const QString &text);
};