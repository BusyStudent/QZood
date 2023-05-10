#include <QApplication>
#include <QMainWindow>

#include "tests/testwindow.hpp"
#include "ui/common/customizeTitleWidget.hpp"
#include "ui/zood/zood.hpp"

int main(int argc, char **argv) {
    QApplication a(argc, argv);
    a.setAttribute(Qt::AA_EnableHighDpiScaling, true);

    ZoodTestWindow twin;
    twin.show();

	Zood zood;
	zood.show();

    return a.exec();
}