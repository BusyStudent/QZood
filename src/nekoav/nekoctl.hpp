#pragma once

#include <QString>
#include <QObject>

namespace NekoAV {

class MediaControl : public QObject {
    // Q_OBJECT
    public:
        MediaControl(QObject *parent = nullptr);
        ~MediaControl();
};


}