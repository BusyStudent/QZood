#pragma once

#include "client.hpp"
#include <QNetworkAccessManager>

#if 0

class YhdmClient : public VideoInterface {
    Q_OBJECT
    public:
        YhdmClient(QObject *parent = nullptr);
        ~YhdmClient();

        /**
         * @brief Do search videos
         * 
         * @param video 
         * @return NetResultPtr<BangumiList> 
         */
        virtual NetResult<BangumiList> searchBangumi(const QString& video) override;
        /**
         * @brief Get the timeline
         * 
         * @return NetResult<Timeline> 
         */
        virtual NetResult<Timeline>    fetchTimeline()                      override;
        virtual QString                name()                               override;
    private:
        QNetworkAccessManager manager;
        QStringList           urls = QStringList(QString("http://www.yinghuacd.com/"));
};

#endif