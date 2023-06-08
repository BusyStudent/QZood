#pragma once

#include <QNetworkAccessManager>
#include "promise.hpp"

class YsjdmBangumi {
    public:

};

/**
 * @brief YSJDM 
 * 
 */
class YsjdmClient : public QObject {
    Q_OBJECT
    public:
        YsjdmClient(QObject *parent = nullptr);
        ~YsjdmClient();
    private:
        QNetworkAccessManager manager;
};