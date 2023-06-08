#pragma once

#include "client.hpp"
#include <QNetworkAccessManager>

class YhdmTimelineItem {
    public:

};

class YhdmClient : public QObject {
    Q_OBJECT
    public:
        YhdmClient(QObject *parent = nullptr);
        ~YhdmClient();

        NetResult<YhdmTimelineItem> fetchTimeline();
    private:
        QNetworkAccessManager manager;
        QStringList           urls = QStringList(QString("http://www.yinghuacd.com/"));
};