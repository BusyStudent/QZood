#pragma once

#include "client.hpp"
#include <QNetworkAccessManager>
#include <QImage>

#define YHDM_CLIENT_NAME u8"樱花动漫"

#if 1

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
        NetResult<BangumiList> searchBangumi(const QString& video) override;
        /**
         * @brief Get the timeline
         * 
         * @return NetResult<Timeline> 
         */
        NetResult<Timeline>    fetchTimeline()                     override;
        QString                name()                              override;
        QString                domain()                            const;

        NetResult<QByteArray>  fetchFile(const QString &url);
        NetResult<QImage>      fetchImage(const QString &url);
    private:
        QNetworkAccessManager  manager;
        QStringList            urls = QStringList(QString("http://www.yinghuacd.com/"));
};

#endif