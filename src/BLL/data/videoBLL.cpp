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

VideoBLLEpisode::VideoBLLEpisode(const EpisodePtr episode) : video(episode) {
    update();
}

QListWidgetItem* VideoBLLEpisode::addToList(QListWidget* listWidget) {
    QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/loading_bar.png"), video->title());
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
    return video->title();
}

QStringList VideoBLLEpisode::sourcesList() {
    return video->sourcesList();
}

QStringList VideoBLLEpisode::danmakuSourceList() {
    return danmakuList + video->danmakuSourceList();
}

QStringList VideoBLLEpisode::subtitleSourceList() {
    return subtitleList;
}

void VideoBLLEpisode::loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) {
    video->fetchVideo(currentVideoSource).then(callAble);
}

void VideoBLLEpisode::loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) {
    video->fetchVideo(currentVideoSource).then(ctxt, callAble);
}

void VideoBLLEpisode::loadThumbnail(std::function<void(const Result<QImage>&)> callAble) {
    if (dirty & THUMBNAIL) {
        video->fetchCover().then([this](const Result<QImage>& img){
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
        video->fetchCover().then(ctxt, [this](const Result<QImage>& img){
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
        video->fetchDanmaku(currentDanmakuSource).then([this](const Result<DanmakuList>& d){
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
        video->fetchDanmaku(currentDanmakuSource).then(ctxt, [this](const Result<DanmakuList>& d){
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

VideoBLLLocal::VideoBLLLocal(const QString filepath) : filePath(filepath) { 
    sourceList.push_back("本地");
    setCurrentVideoSource("本地");
}

QListWidgetItem* VideoBLLLocal::addToList(QListWidget* listWidget) {
    QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/loading_bar.png"), QFileInfo(filePath).fileName());
    listWidget->addItem(item);
    return item;
}

QString VideoBLLLocal::title() {
    return QFileInfo(filePath).fileName();
}

void VideoBLLLocal::loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) {
    callAble(filePath);
}

void VideoBLLLocal::loadVideoToPlay(QObject*, std::function<void(const Result<QString>&)> callAble) {
    loadVideoToPlay(callAble);
}

void VideoBLLLocal::loadThumbnail(std::function<void(const Result<QImage>&)> callAble) {
    callAble(QImage());
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
