#include "../../nekoav/nekoutils.hpp"
#include "videoBLL.hpp"

#include <QFileInfo>

QStringList VideoBLL::sourcesList() {
    return sourceList;
}

QStringList VideoBLL::danmakuSourceList() {
    return danmakuList;
}

QStringList VideoBLL::subtitleSourceList() {
    return subtitleList;
}

void VideoBLL::loadDanmakuFromFile(const QString &filepath) {
    danmakuList.push_back(filepath);
}

void VideoBLL::loadSubtitleFromFile(const QString &filepath) {
    subtitleList.push_back(filepath);
}

void VideoBLL::setCurrentVideoSource(const QString& source) {
    if (source.isEmpty() || sourcesList().contains(source)) {
        currentVideoSource = source;
    }
}

void VideoBLL::addSource(const QString& source) {
    sourceList.push_back(source);
}

void VideoBLL::addDanmakuSource(const QString& source) {
    danmakuList.push_back(source);
}

void VideoBLL::addSubtitleSource(const QString& source) {
    subtitleList.push_back(source);
}

void VideoBLL::setCurrentSubtitleSource(const QString& source) {
    if (source.isEmpty() || subtitleSourceList().contains(source)) {
        currentSubtitleSource = source;
    }
}

void VideoBLL::setCurrentDanmakuSource(const QString& source) {
    if (source.isEmpty() || danmakuSourceList().contains(source)) {
        currentDanmakuSource = source;
        dirty &= (uint16_t)DANMAKU;
    }
}

VideoBLLEpisode::VideoBLLEpisode(const EpisodeList episodes) : videos(episodes) {
    update();
    setCurrentVideoSource(sourcesList().size() > 0 ? sourcesList()[0] : "");
}

VideoBLLPtr VideoBLLEpisode::operator+(const VideoBLLPtr video) {
    auto v = dynamic_cast<const VideoBLLEpisode *>(video.get());
    EpisodeList episodes = videos + (v == nullptr ? EpisodeList() : v->videos);
    return createVideoBLL(episodes);
}

QListWidgetItem* VideoBLLEpisode::addToList(QListWidget* listWidget) {
    QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/loading_bar.png"), title());
    loadThumbnail([item](const Result<QImage> &image){
        if (image.has_value()) {
            item->setIcon(QPixmap::fromImage(image.value()));
        } else {
            item->setIcon(QIcon());
        }
    });
    listWidget->addItem(item);
    return item;
}

QString VideoBLLEpisode::title() {
    if (videos.isEmpty()) {
        return "";
    }
    return videos[0]->title();
}

QString VideoBLLEpisode::longTitle() {
    if (videos.isEmpty()) {
        return "";
    }
    return videos[0]->longTitle();
}

QString VideoBLLEpisode::indexTitle() {
    if (videos.isEmpty()) {
        return "";
    }
    return videos[0]->indexTitle();
}

QStringList VideoBLLEpisode::sourcesList() {
    if (VIDEOSOURCE & dirty) {
        dirty ^= VIDEOSOURCE;
        sourceList.clear();
        for (int index = 0;index < videos.size(); ++index) {
            for (auto& sourceName : videos[index]->sourcesList()) { 
                sourceList.push_back(sourceName);
                mapVideoSourceName.insert(sourceList.back(), qMakePair(index, sourceName));
            }
        }
    }
    return sourceList;
}

QStringList VideoBLLEpisode::danmakuSourceList() {
    if (DANMAKUSOURCE & dirty) {
        dirty ^= DANMAKUSOURCE;
        danmakuList.clear();
        for (int index = 0;index < videos.size(); ++index) {
            for (auto& sourceName : videos[index]->danmakuSourceList()) {
                danmakuList.push_back(sourceName);
                mapDanmakuSourceName.insert(danmakuList.back(), qMakePair(index, sourceName));
            }
        }
    }
    return danmakuList;
}

QStringList VideoBLLEpisode::subtitleSourceList() {
    return subtitleList;
}

void VideoBLLEpisode::loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) {
    videos[mapVideoSourceName[currentVideoSource].first]->fetchVideo(mapVideoSourceName[currentVideoSource].second).then(callAble);
}

void VideoBLLEpisode::loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) {
    qDebug() << "fetch vedio " << title() << " " << mapVideoSourceName[currentVideoSource].first << " in source " << mapVideoSourceName[currentVideoSource].second;
    videos[mapVideoSourceName[currentVideoSource].first]->fetchVideo(mapVideoSourceName[currentVideoSource].second).then(ctxt, callAble);
}

void VideoBLLEpisode::loadThumbnail(std::function<void(const Result<QImage>&)> callAble) {
    if (dirty & THUMBNAIL) {
        videos[mapVideoSourceName[currentVideoSource].first]->fetchCover().then([this](const Result<QImage>& img){
            if(img.has_value()) {
                thumbnail = img.value();
            }
        });
        dirty = dirty ^ (uint16_t)THUMBNAIL;
    }
    callAble(thumbnail);
}

void VideoBLLEpisode::loadThumbnail(QObject *ctxt, std::function<void(const Result<QImage>&)> callAble) {
    if (dirty & THUMBNAIL) {
        videos[mapVideoSourceName[currentVideoSource].first]->fetchCover().then(ctxt, [this](const Result<QImage>& img){
            if(img.has_value()) {
                thumbnail = img.value();
            }
        });
        dirty = dirty ^ (uint16_t)THUMBNAIL;
    }
    callAble(thumbnail);
}

void VideoBLLEpisode::loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble) {
    if (dirty & THUMBNAIL) {
        videos[mapDanmakuSourceName[currentDanmakuSource].first]->fetchDanmaku(mapDanmakuSourceName[currentDanmakuSource].second).then([this](const Result<DanmakuList>& d){
            if(d.has_value()) {
                danmaku = d.value();
            }
        });
        dirty = dirty ^ (uint16_t)DANMAKU;
    }
    callAble(danmaku);
}

void VideoBLLEpisode::loadDanmaku(QObject *ctxt, std::function<void(const Result<DanmakuList>&)> callAble) {
    if (dirty & THUMBNAIL) {
        videos[mapDanmakuSourceName[currentDanmakuSource].first]->fetchDanmaku(mapDanmakuSourceName[currentDanmakuSource].second).then(ctxt, [this](const Result<DanmakuList>& d){
            if(d.has_value()) {
                danmaku = d.value();
            }
        });
        dirty = dirty ^ (uint16_t)DANMAKU;
    }
    callAble(danmaku);
}

void VideoBLLEpisode::loadSubtitle(std::function<void(const Result<QString>&)> callAble) {
    callAble(currentSubtitleSource);
}

void VideoBLLEpisode::loadSubtitle(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) {
    loadSubtitle(callAble);
}

VideoBLLLocal::VideoBLLLocal(const QStringList filepaths) : filePaths(filepaths) { 
    for (int i = 0;i < filepaths.size(); ++i) {
        sourceList.push_back(QString("本地%1").arg(i + 1));
    }
    if (filepaths.size() > 0) {
        setCurrentVideoSource(sourceList.front());
    }
}

VideoBLLPtr VideoBLLLocal::operator+(const VideoBLLPtr v) {
    auto vl = dynamic_cast<const VideoBLLLocal *>(v.get());
    auto files = filePaths + (vl == nullptr ? QStringList() : vl->filePaths);
    return std::shared_ptr<VideoBLLLocal>(new VideoBLLLocal(files));
}

QListWidgetItem* VideoBLLLocal::addToList(QListWidget* listWidget) {
    QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/loading_bar.png"), title());
    loadThumbnail([item](const Result<QImage> &image){
        if (image.has_value()) {
            item->setIcon(QPixmap::fromImage(image.value()));
        } else {
            item->setIcon(QIcon());
        }
    });
    listWidget->addItem(item);
    return item;
}

QString VideoBLLLocal::title() {
    return sourceList.size() == 0 ? "" : QFileInfo(filePaths.front()).fileName();
}

QString VideoBLLLocal::longTitle() {
    return sourceList.size() == 0 ? "" : QFileInfo(filePaths.front()).fileName();
}

QString VideoBLLLocal::indexTitle() {
    return sourceList.size() == 0 ? "" : QFileInfo(filePaths.front()).fileName();
}

void VideoBLLLocal::loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) {
    if (sourceList.contains(currentVideoSource)) {
        callAble(filePaths[sourceList.indexOf(currentVideoSource)]);
    } else {
        callAble(Result<QString>());
    }
}

void VideoBLLLocal::loadVideoToPlay(QObject*, std::function<void(const Result<QString>&)> callAble) {
    loadVideoToPlay(callAble);
}

void VideoBLLLocal::loadThumbnail(std::function<void(const Result<QImage>&)> callAble) {
    if (sourceList.contains(currentVideoSource)) {
        callAble(NekoAV::GetMediaFileIcon(filePaths[sourceList.indexOf(currentVideoSource)]));
    } else {
        callAble(Result<QImage>());
    }
}

void VideoBLLLocal::loadThumbnail(QObject*, std::function<void(const Result<QImage>&)> callAble) {
    loadThumbnail(callAble);
}

void VideoBLLLocal::loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble) {
    if (!currentDanmakuSource.isEmpty() && danmakuList.size() > 0) {
        callAble(danmaku);
    }
}

void VideoBLLLocal::loadDanmaku(QObject*, std::function<void(const Result<DanmakuList>&)> callAble) {
    loadDanmaku(callAble);
}

void VideoBLLLocal::loadSubtitle(std::function<void(const Result<QString>&)> callAble) {
    if (!currentSubtitleSource.isEmpty()) {
        callAble(currentSubtitleSource);
    }
}

void VideoBLLLocal::loadSubtitle(QObject*, std::function<void(const Result<QString>&)> callAble) {
    loadSubtitle(callAble);
}
