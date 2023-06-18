#pragma once

#include <QString>
#include <QListWidget>

#include "../../net/datalayer.hpp"

class VideoBLL;

using VideoBLLPtr = RefPtr<VideoBLL>;

template <typename SourceT>
inline VideoBLLPtr createVideoBLL(const SourceT) {
    class __ShouldNotBeUsedStruct {};
    static_assert(std::is_same_v<SourceT,__ShouldNotBeUsedStruct>, "对应类型的视频资源转换没有实现！");
}

class VideoBLL : public DynRefable {
    public:
        enum DataItem : uint16_t {
            THUMBNAIL = 1<<0,
            DANMAKU = 1<<1,

            ALL = 0xffff,
        };
    public:
        virtual QListWidgetItem* addToList(QListWidget* listWidget) = 0;
        virtual QString title() = 0;

        virtual void loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) = 0;
        virtual void loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) = 0;
        virtual void loadThumbnail(std::function<void(const Result<QImage>&)> callAble) = 0;
        virtual void loadThumbnail(QObject *ctxt, std::function<void(const Result<QImage>&)> callAble) = 0;
        virtual void loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble) = 0;
        virtual void loadDanmaku(QObject *ctxt, std::function<void(const Result<DanmakuList>&)> callAble) = 0;
        virtual void loadSubtitle(std::function<void(const Result<QString>&)> callAble) = 0;
        virtual void loadSubtitle(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) = 0;

        virtual QStringList sourcesList();
        virtual QStringList danmakuSourceList();
        virtual QStringList subtitleSourceList();

        virtual void loadDanmakuFromFile(const QString &filepath);
        virtual void loadSubtitleFromFile(const QString &filepath);

        virtual void setCurrentVideoSource(const QString& source);
        virtual void setCurrentSubtitleSource(const QString& source);
        virtual void setCurrentDanmakuSource(const QString& source);

        inline void update(DataItem f = ALL) { dirty = f; }
        template<typename ValueT>
        inline void setStatus(const QString& key,const ValueT& value) {
            status.insert(key, value);
        }
        template<typename ValueT>
        inline ValueT getStatus(const QString& key) {
            return status[key].value<ValueT>();
        }
        inline bool containsStatus(const QString& key) {
            return status.contains(key);
        }

    protected:
        VideoBLL() = default;
    
    protected:
        // 视频资源数据来源列表
        QStringList sourceList;
        QStringList danmakuList;
        QStringList subtitleList;
        // 本视频当前申请的源
        QString currentVideoSource = "";
        QString currentSubtitleSource = "";
        QString currentDanmakuSource = "";
        // 本地缓存
        uint16_t dirty;
        QImage thumbnail;
        DanmakuList danmaku;
    public:
        // 需要保留的视频播放状态设置
        QMap<QString, QVariant> status;

};

class VideoBLLEpisode : public VideoBLL {
    public:
        QListWidgetItem* addToList(QListWidget* listWidget) override;
        QString title() override;

        QStringList sourcesList() override;
        QStringList danmakuSourceList() override;
        QStringList subtitleSourceList() override;
        
        void loadVideoToPlay(std::function<void(const Result<QString>&)> callAble);
        void loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble);
        void loadThumbnail(std::function<void(const Result<QImage>&)> callAble);
        void loadThumbnail(QObject *ctxt, std::function<void(const Result<QImage>&)> callAble);
        void loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble);
        void loadDanmaku(QObject *ctxt, std::function<void(const Result<DanmakuList>&)> callAble);
        void loadSubtitle(std::function<void(const Result<QString>&)> callAble);
        void loadSubtitle(QObject *ctxt, std::function<void(const Result<QString>&)> callAble);

    protected:
        VideoBLLEpisode(const EpisodePtr episode);

    private:
        EpisodePtr video = nullptr;

    friend VideoBLLPtr createVideoBLL<>(const EpisodePtr);
};

// TODO(llhsdmd) : 等接口,先存地址做本地测试
class VideoBLLLocal : public VideoBLL {
    public:
        QListWidgetItem* addToList(QListWidget* listWidget) override;
        QString title() override;

        void loadVideoToPlay(std::function<void(const Result<QString>&)> callAble);
        void loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble);
        void loadThumbnail(std::function<void(const Result<QImage>&)> callAble);
        void loadThumbnail(QObject *ctxt, std::function<void(const Result<QImage>&)> callAble);
        void loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble);
        void loadDanmaku(QObject *ctxt, std::function<void(const Result<DanmakuList>&)> callAble);
        void loadSubtitle(std::function<void(const Result<QString>&)> callAble);
        void loadSubtitle(QObject *ctxt, std::function<void(const Result<QString>&)> callAble);

    protected:
        VideoBLLLocal(const QString filepath);

    private:
        QString filePath;

    friend VideoBLLPtr createVideoBLL<>(const QString);
};

template <>
inline VideoBLLPtr createVideoBLL<QString>(const QString filepath) {
    // return std::make_shared<VideoBLLLocal>(filepath);
    return VideoBLLPtr(new VideoBLLLocal(filepath));
}

template <>
inline VideoBLLPtr createVideoBLL<EpisodePtr>(const EpisodePtr episode) {
    //  return std::make_shared<VideoBLL>(episode);
    return VideoBLLPtr(new VideoBLLEpisode(episode));
}