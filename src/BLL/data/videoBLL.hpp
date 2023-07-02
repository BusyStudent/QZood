#pragma once

#include <QString>
#include <QListWidget>

#include "../../net/datalayer.hpp"

class VideoBLL;

using VideoBLLPtr = RefPtr<VideoBLL>;
using VideoBLLList = QList<VideoBLLPtr>;

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
            VIDEOSOURCE = 1<<2,
            DANMAKUSOURCE = 1<<3,

            ALL = 0xffff,
        };
    public:
        virtual QListWidgetItem* addToList(QListWidget* listWidget) = 0;
        virtual QString title() = 0;
        virtual QString longTitle() = 0;
        virtual QString indexTitle() = 0;

        virtual void loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) = 0;
        virtual void loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) = 0;
        virtual void loadThumbnail(std::function<void(const Result<QImage>&)> callAble) = 0;
        virtual void loadThumbnail(QObject *ctxt, std::function<void(const Result<QImage>&)> callAble) = 0;
        virtual void loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble) = 0;
        virtual void loadDanmaku(QObject *ctxt, std::function<void(const Result<DanmakuList>&)> callAble) = 0;
        virtual void loadSubtitle(std::function<void(const Result<QString>&)> callAble) = 0;
        virtual void loadSubtitle(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) = 0;

        virtual VideoBLLPtr operator+(const VideoBLLPtr) = 0;

        virtual QStringList sourcesList();
        virtual QStringList danmakuSourceList();
        virtual QStringList subtitleSourceList();

        virtual void addSource(const QString& source);
        virtual void addDanmakuSource(const QString& source);
        virtual void addSubtitleSource(const QString& source);

        virtual void loadDanmakuFromFile(const QString &filepath);
        virtual void loadSubtitleFromFile(const QString &filepath);

        virtual void setCurrentVideoSource(const QString& source);
        virtual void setCurrentSubtitleSource(const QString& source);
        virtual void setCurrentDanmakuSource(const QString& source);

        virtual inline QString getCurrentVideoSource() { return currentVideoSource; }
        virtual inline QString getCurrentDanmakuSource() { return currentDanmakuSource; }
        virtual inline QString getCurrentSubtitleSource() { return currentSubtitleSource; }

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
        QString longTitle() override;
        QString indexTitle() override;

        QStringList sourcesList() override;
        QStringList danmakuSourceList() override;
        QStringList subtitleSourceList() override;
        
        void loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) override;
        void loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) override;
        void loadThumbnail(std::function<void(const Result<QImage>&)> callAble) override;
        void loadThumbnail(QObject *ctxt, std::function<void(const Result<QImage>&)> callAble) override;
        void loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble) override;
        void loadDanmaku(QObject *ctxt, std::function<void(const Result<DanmakuList>&)> callAble) override;
        void loadSubtitle(std::function<void(const Result<QString>&)> callAble) override;
        void loadSubtitle(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) override;

        VideoBLLPtr operator+(const VideoBLLPtr) override;

    protected:
        VideoBLLEpisode(const EpisodeList episode = EpisodeList());

    private:
        EpisodeList videos;
        QMap<QString, QPair<int, QString>> mapVideoSourceName;
        QMap<QString, QPair<int, QString>> mapDanmakuSourceName;

    friend VideoBLLPtr createVideoBLL<>(const EpisodeList);
};

// TODO(llhsdmd) : 等接口,先存地址做本地测试
class VideoBLLLocal : public VideoBLL {
    public:
        QListWidgetItem* addToList(QListWidget* listWidget) override;
        QString title() override;
        QString longTitle() override;
        QString indexTitle() override;

        void loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) override;
        void loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) override;
        void loadThumbnail(std::function<void(const Result<QImage>&)> callAble) override;
        void loadThumbnail(QObject *ctxt, std::function<void(const Result<QImage>&)> callAble) override;
        void loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble) override;
        void loadDanmaku(QObject *ctxt, std::function<void(const Result<DanmakuList>&)> callAble) override;
        void loadSubtitle(std::function<void(const Result<QString>&)> callAble) override;
        void loadSubtitle(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) override;

        virtual VideoBLLPtr operator+(const VideoBLLPtr) override;

    protected:
        VideoBLLLocal(const QStringList filepaths);

    private:
        QStringList filePaths;

    friend VideoBLLPtr createVideoBLL<>(const QString);
};

template <>
inline VideoBLLPtr createVideoBLL<QString>(const QString filepath) {
    // return std::make_shared<VideoBLLLocal>(filepath);
    return VideoBLLPtr(new VideoBLLLocal(QStringList{filepath}));
}

template <>
inline VideoBLLPtr createVideoBLL<EpisodeList>(const EpisodeList episodes) {
    //  return std::make_shared<VideoBLL>(episode);
    return VideoBLLPtr(new VideoBLLEpisode(episodes));
}