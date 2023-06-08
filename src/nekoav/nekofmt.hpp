#pragma once

#include <QObject>
#include <QString>

namespace NekoAV {

class DashInputFormatPrivate;
class DashInputFormat : public QObject {
    Q_OBJECT;
    public:
        DashInputFormat(QObject *parent);
        ~DashInputFormat();

        void *getAVInputFormat();
        void  setAudioSource(const QUrl &url);
        void  setVideoSource(const QUrl &url);
        void  setAudioOption(const QByteArray &key, const QByteArray &value);
        void  setVideoOption(const QByteArray &key, const QByteArray &value);
        void  clearVideoOption();
        void  clearAudioOption();
    private:
        QScopedPointer<DashInputFormatPrivate> d;
};

}

using NekoDashInputFormat = NekoAV::DashInputFormat;