#pragma once

#include <QMainWindow>
#include <functional>
#include <QTreeWidgetItem>

#include "../log.hpp"

#define ZOOD_TEST(name) static QWidget *test__##name(); \
    static bool test__init_##name = []() {              \
        ZoodRegisterTest(#name, test__##name);          \
        return true;                                    \
    }();                                                \
    static QWidget *test__##name()                      

class ZoodTestWindow : public QMainWindow {
    Q_OBJECT
    public:
        
        ZoodTestWindow(QWidget *parent = nullptr);
        ~ZoodTestWindow();
        QWidget* TerminatorParent();

		public Q_SLOTS:
		void ItemClicked(QTreeWidgetItem *item, int column);

    private:
        void *ui = nullptr;
		QMap<QTreeWidgetItem*, QWidget*> items;
		QTreeWidgetItem* currentItem = nullptr;
    friend void ZoodLogString(const QString &text);
};

void ZoodRegisterTest(const QString &name, const std::function<QWidget*()> &create);
void ZoodLogString(const QString &text);