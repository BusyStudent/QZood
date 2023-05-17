#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include "client.hpp"

class BiliVideoInfo final {
    public:
        QString bvid;
        QString aid;
};
class BiliVideoSource final {
    public:
        QStringList urls;
        QByteArray referHeader; //< Header of refer
};

class BiliClient final : public VideoInterface {
    Q_OBJECT
    public:
        BiliClient(QObject *parent = nullptr);
        ~BiliClient();

        /**
         * @brief fetch danmaku by cid
         * 
         * @param cid 
         * @return The async result of this request
         */
        NetResult<DanmakuList> fetchDanmaku(const QString &cid);
        /**
         * @brief fetch danmaku xml file
         * 
         * @param cid 
         * @return NetResult<DanmakuList> 
         */
        NetResult<QByteArray> fetchDanmakuXml(const QString &cid);
        /**
         * @brief fetch danmaku protobuf
         * 
         * @param cid 
         * @return NetResult<QByteArray> 
         */
        NetResult<DanmakuList> fetchDanmakuProtobuf(const QString &cid, int segment);
        /**
         * @brief Get suggestions
         * 
         * @param text 
         * @return NetResult<QStringList> 
         */
        NetResult<QStringList> fetchSearchSuggestions(const QString &text);
        /**
         * @brief fetch file by
         * 
         * @param url 
         * @return NetResult<QByteArray> 
         */
        NetResult<QByteArray>  fetchFile(const QString &url);
        /**
         * @brief fetch danmaku video info
         * 
         * @param bvid The bvid of the video
         * @return The async result of this request
         */
        NetResult<BiliVideoInfo> fetchVideoInfo(const QString &bvid);
        /**
         * @brief Get video source
         * 
         * @param cid
         * @param bvid 
         * @return NetResult<BiliVideoSource> 
         */
        NetResult<BiliVideoSource> fetchVideoSource(const QString &cid, const QString &bvid);
        /**
         * @brief Convert bvid or avid to cid
         * 
         * @param bvid 
         * @return The async result of this request 
         */
        NetResult<QString> convertToCid(const QString &bvid);
    public:
        /**
         * @brief Get BVID from URL
         * 
         * @param url The url of it
         * @return Result<QString> 
         */
        Result<QString> parseBvid(const QString &url);
    private:
        void _on_videoCidReplyReady(NetResult<QString> &result, QNetworkReply *reply);

        QNetworkAccessManager manager; //< Manager to access it
};