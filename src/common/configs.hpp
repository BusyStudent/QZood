#pragma once

#include <QSettings>

class Configs {
 public:
    static Configs* instace();
    static QSettings* settings();
    static QString configPath();
 private:
    Configs();
 private:
    static QSettings mSettings;
    const static QString mPath;
    const static QString mHistoryFile;
    const static QString mSetttingsFile;  
};