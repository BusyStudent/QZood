#include <QApplication>
#include "./ui/zood/zood.hpp"

int main(int argc, char **argv) {
    QApplication a(argc, argv);
    Zood zood;
    zood.show();
    return a.exec();
}