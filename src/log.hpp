#pragma once
#include <QString>

#define ZOOD_CLOG(...) ZoodLogString(QString::asprintf(__VA_ARGS__))

void ZoodLogString(const QString &text);