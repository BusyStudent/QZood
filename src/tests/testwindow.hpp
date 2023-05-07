#pragma once

#include <QMainWindow>
#include <functional>

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
    private:
        void *ui = nullptr;
    friend void ZoodLogString(const QString &text);
};

void ZoodRegisterTest(const QString &name, const std::function<QWidget*()> &create);
void ZoodLogString(const QString &text);