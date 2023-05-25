#pragma once

#include <memory>
#include "promise.hpp"

/**
 * @brief 
 * 
 */
class VideoSource : public std::enable_shared_from_this<VideoSource> {
    public:

        /**
         * @brief Get detail url
         * 
         * @return NetResult<QString> 
         */
        virtual NetResult<QString> url(size_t index = 0) = 0;
    protected:
        virtual ~VideoSource() = default;
};
/**
 * @brief Interface for Video 
 * 
 */
class VideoList  : public std::enable_shared_from_this<VideoList> {
    public:
        virtual NetResultPtr<VideoSource> fechVideoSource() = 0;
        virtual NetResult<QImage>         cover() = 0;
        virtual QString                   name() = 0;
        virtual QString                   description() = 0;
    protected:
        virtual ~VideoList() = default;
};
/**
 * @brief Interface for searching video sources
 * 
 */
class VideoInterface {
    public:
        /**
         * @brief Do search videos
         * 
         * @param video 
         * @return NetResultPtr<VideoList> 
         */
        virtual NetResultPtr<VideoList> searchVideo(const QString& video) = 0;
        virtual QString                 name()                            = 0;
    protected:
        virtual ~VideoInterface() = default;
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