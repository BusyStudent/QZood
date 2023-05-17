#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <memory>

#include "../danmaku.hpp"
#include "../stl.hpp"

class NetPromiseHelper : public QObject {
    Q_OBJECT
    public:
        NetPromiseHelper(QObject *parent = nullptr);
        ~NetPromiseHelper();

        void doNotify(const void *data);
    signals:
        void notify(const void *data);
};

/**
 * @brief Net Promise
 * 
 * @tparam T 
 */
template <typename T>
class NetPromise {
    public:
        NetPromise() = default;
        NetPromise(const NetPromise &) = default;
        NetPromise(NetPromise &&) = default;
        ~NetPromise() = default;

        template <typename Callable>
        NetPromise &then(Callable &&cb) {
            QObject::connect(helper.get(), &NetPromiseHelper::notify, [cb](const void *v) mutable {
                cb(*static_cast<const T*>(v));
            });
            return *this;
        }
        template <typename Callable>
        NetPromise &then(QObject *ctxt, Callable &&cb) {
            QObject::connect(helper.get(), &NetPromiseHelper::notify, ctxt, [cb](const void *v) mutable {
                cb(*static_cast<const T*>(v));
            });
            return *this;
        }

        /**
         * @brief Notify the backend of this result
         * 
         * @param value 
         */
        NetPromise &putResult(const T &value) {
            helper->doNotify(&value);
            return *this;
        }
        NetPromise &putLater(const T &value) {
            QMetaObject::invokeMethod(helper.get(), [h = this->helper, value]() {
                h->doNotify(&value);
            }, Qt::QueuedConnection);
            return *this;
        }

        /**
         * @brief Create a new Net Promise
         * 
         * @return NetPromise 
         */
        static NetPromise Alloc() {
            return std::make_shared<NetPromiseHelper>();
        }

        NetPromise &operator =(const NetPromise &) = default;
        NetPromise &operator =(NetPromise &&) = default;
    private:
        NetPromise(std::shared_ptr<NetPromiseHelper> &&h) : helper(h) { }
        
        std::shared_ptr<NetPromiseHelper> helper;
};
template <typename T>
using AsyncResult = NetPromise<Result<T>>;
template <typename T>
using NetResult = NetPromise<Result<T>>;

// Video Interface
class VideoSource     : public QObject {
    Q_OBJECT
    public:
        using QObject::QObject;
};
class VideoCollection : public QObject {
    Q_OBJECT
    public:
        using QObject::QObject;
};
class VideoInterface : public QObject  {
    Q_OBJECT
    public:
        using QObject::QObject;
};


/**
 * @brief Get An random useragent
 * 
 * @return QByteArray 
 */
QByteArray RandomUserAgent();