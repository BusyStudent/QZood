#pragma once
#include <QString>
#include <QDebug>

#define ZOOD_CLOG(...)      ZoodLogString(QString::asprintf(__VA_ARGS__))
#define ZOOD_QLOG(fmt, ...) ZoodLogString(QString(fmt).arg(__VA_ARGS__))

class ZoodDebug {

};

void ZoodLogString(const QString &text);