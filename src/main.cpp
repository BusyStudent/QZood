#include <QApplication>
#include <QMainWindow>

#include "tests/testwindow.hpp"

int main(int argc, char **argv) {
    QApplication a(argc, argv);
    a.setAttribute(Qt::AA_EnableHighDpiScaling, true);

    ZoodTestWindow twin;
    twin.show();

    return a.exec();
}