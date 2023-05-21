#pragma once

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <memory>

#include "../danmaku.hpp"
#include "../stl.hpp"

class NetPromiseHelper : public QObject, public std::enable_shared_from_this<NetPromiseHelper> {
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
        template <typename Object, typename RetT, typename ...Args>
        NetPromise &then(Object *ctxt, RetT (Object::*fn)(Args ...)) {
            QObject::connect(helper.get(), &NetPromiseHelper::notify, ctxt, [ctxt, fn](const void *v) mutable {
                (ctxt->*fn)(*static_cast<const T*>(v));
            });
            return *this;
        }
        template <typename Object, typename RetT, typename ...Args>
        NetPromise &then(RetT (Object::*fn)(Args ...), Object *ctxt) {
            return then(ctxt, fn);
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
        /**
         * @brief Notify the backend of this result with Qt::QueuedConnection 
         * 
         * @param value 
         * @return NetPromise& 
         */
        NetPromise &putLater(const T &value) {
            putResult(value, Qt::QueuedConnection);
            return *this;
        }
        /**
         * @brief Notify the backend of this result with giving con type
         * 
         * @param value 
         * @param type 
         * @return NetPromise& 
         */
        NetPromise &putResult(const T &value, Qt::ConnectionType type) {
            QMetaObject::invokeMethod(helper.get(), [h = this->helper, value]() {
                h->doNotify(&value);
            }, type);
            return *this;
        }
        /**
         * @brief Cancel it
         * 
         * @return NetPromise& 
         */
        NetPromise &cancel() {
            helper->disconnect();
            return *this;
        }
        NetPromise &moveToThread(QThread *thread) {
            helper->moveToThread(thread);
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
        NetPromise(RefPtr<NetPromiseHelper> &&h) : helper(h) { }
        
        RefPtr<NetPromiseHelper> helper;
};
template <typename T>
using AsyncResult = NetPromise<Result<T> >;
template <typename T>
using NetResult = NetPromise<Result<T> >;
template <typename T>
using NetResultPtr = NetPromise<ResultPtr<T> >;

/**
 * @brief 
 * 
 */
class VideoSource : public QObject {
    Q_OBJECT
    public:
        using QObject::QObject;

        /**
         * @brief Get detail url
         * 
         * @return NetResult<QString> 
         */
        virtual NetResult<QString> url(size_t index = 0) = 0;
};
/**
 * @brief Interface for Video 
 * 
 */
class VideoList  : public QObject {
    Q_OBJECT
    public:
        using QObject::QObject;

        virtual NetResultPtr<VideoSource> fechVideoSource() = 0;
        virtual NetResult<QImage>         cover() = 0;
        virtual QString                   name() = 0;
        virtual QString                   description() = 0;
};
/**
 * @brief Interface for searching video sources
 * 
 */
class VideoInterface : public QObject  {
    Q_OBJECT
    public:
        using QObject::QObject;

        /**
         * @brief Do search videos
         * 
         * @param video 
         * @return NetResultPtr<VideoList> 
         */
        virtual NetResultPtr<VideoList> searchVideo(const QString& video) = 0;
        virtual QString                 name()                            = 0;
};


#define ZOOD_VIDEO_INTERFACE(name) \
    static bool video__init_##name = []() {              \
        RegisterVideoInterface([]() -> VideoInterface *{ \
            return new name();                           \
        });                                              \
        return true;                                     \
    }();                           

/**
 * @brief Get An random useragent
 * 
 * @return QByteArray 
 */
QByteArray RandomUserAgent();
void       RegisterVideoInterface(VideoInterface *(*fn)());
void       InitializeVideoInterface();
QList<VideoInterface*> &GetVideoInterfaceList();