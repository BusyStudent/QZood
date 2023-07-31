#include "configs.hpp"

#include <memory>
#include <QApplication>

const QString Configs::mPath = QApplication::applicationDirPath() + "/configs";

Configs* Configs::instace() {
    static std::unique_ptr<Configs> d(new Configs());
    return d.get();
}

QSettings* Configs::settings() {
    return (&mSettings);
}

QString Configs::configPath() {
    return mPath;
}