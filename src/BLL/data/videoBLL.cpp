#include "../../nekoav/nekoutils.hpp"
#include "videoBLL.hpp"
#include "../../common/myGlobalLog.hpp"

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
        setStatus("currentVideoSource", source);
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
        setStatus("currentSubtitleSource", source);
    }
}

void VideoBLL::setCurrentDanmakuSource(const QString& source) {
    if (source.isEmpty() || danmakuSourceList().contains(source)) {
        setStatus("currentDanmakuSource", source);
    }
}

VideoBLLEpisode::VideoBLLEpisode(const EpisodeList episodes) : videos(episodes) {
    updateSourceList();
    setCurrentVideoSource(sourceList.size() > 0 ? sourceList[0] : "");
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
    return sourceList;
}

void VideoBLLEpisode::updateSourceList() {
    for (int index = 0;index < videos.size(); ++index) {
        for (auto& sourceName : videos[index]->sourcesList()) { 
            if(!sourceList.contains(sourceName)) {
                sourceList.push_back(sourceName);
                mapVideoSourceName.insert(sourceName, qMakePair(index, sourceName));
            }
        }
    }
    if (0 == danmakuList.size()) {
        danmakuList.push_back("无");
        mapDanmakuSourceName.insert(danmakuList.back(), qMakePair(-1, "无"));
    }
    for (int index = 0;index < videos.size(); ++index) {
        for (auto& sourceName : videos[index]->danmakuSourceList()) {
            if (!danmakuList.contains(sourceName)) {
                danmakuList.push_back(sourceName);
                mapDanmakuSourceName.insert(sourceName, qMakePair(index, sourceName));
            }
        }
    }
    if (0 == subtitleList.size()) {
        subtitleList.push_back("无");
    }
    if (!danmakuList.contains(getCurrentDanmakuSource())) {
        setCurrentDanmakuSource(danmakuList.at(0));
    }
    if (!subtitleList.contains(getCurrentSubtitleSource())) {
        setCurrentSubtitleSource(subtitleList.at(0));
    }
}

QStringList VideoBLLEpisode::danmakuSourceList() {
    return danmakuList;
}

QStringList VideoBLLEpisode::subtitleSourceList() {
    return subtitleList;
}

void VideoBLLEpisode::loadVideoToPlay(std::function<void(const Result<QString>&)> callAble) {
    auto source = mapVideoSourceName[getCurrentVideoSource()];
    if (source.first >= 0 && source.first < videos.size()) {
        videos[source.first]->fetchVideo(source.second).then(callAble);
    }
}

void VideoBLLEpisode::loadVideoToPlay(QObject *ctxt, std::function<void(const Result<QString>&)> callAble) {
    auto source = mapVideoSourceName[getCurrentVideoSource()];
    if (source.first >= 0 && source.first < videos.size()) {
        videos[source.first]->fetchVideo(source.second).then(ctxt, callAble);
    }
}

void VideoBLLEpisode::loadThumbnail(std::function<void(const Result<QImage>&)> callAble) {
    auto source = mapVideoSourceName[getCurrentVideoSource()];
    if (source.first >= 0 && source.first < videos.size()) {
        videos[source.first]->fetchCover().then(callAble);
    }
}

void VideoBLLEpisode::loadThumbnail(QObject *ctxt, std::function<void(const Result<QImage>&)> callAble) {
    auto source = mapVideoSourceName[getCurrentVideoSource()];
    if (source.first >= 0 && source.first < videos.size()) {
        videos[source.first]->fetchCover().then(ctxt, callAble);
    }
}

void VideoBLLEpisode::loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble) {
    auto source = mapDanmakuSourceName[getCurrentDanmakuSource()];
    if (source.first >= 0 && source.first < videos.size()) {
        videos[source.first]->fetchDanmaku(source.second).then(callAble);
    } else {
        LOG(DEBUG) << "load danmaku : " << getCurrentDanmakuSource() << ", " << source.first;
    }
}

void VideoBLLEpisode::loadDanmaku(QObject *ctxt, std::function<void(const Result<DanmakuList>&)> callAble) {
    auto source = mapDanmakuSourceName[getCurrentDanmakuSource()];
    if (source.first >= 0 && source.first < videos.size()) {
        videos[source.first]->fetchDanmaku(source.second).then(ctxt, callAble);
    } else {
        LOG(DEBUG) << "load danmaku : " << getCurrentDanmakuSource() << ", " << source.first;
    }
}

void VideoBLLEpisode::loadSubtitle(std::function<void(const Result<QString>&)> callAble) {
    auto subtitle = getCurrentSubtitleSource();
    if (!subtitle.isEmpty() && subtitle != "无") {
        callAble(subtitle);
    }
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
        updateSourceList();
    }
}

void VideoBLLLocal::updateSourceList() {
    if (0 == danmakuList.size()) {
        danmakuList.push_back("无");
    }
    if (0 == subtitleList.size()) {
        subtitleList.push_back("无");
    }
    if (!danmakuList.contains(getCurrentDanmakuSource())) {
        setCurrentDanmakuSource(danmakuList.at(0));
    }
    if (!subtitleList.contains(getCurrentSubtitleSource())) {
        setCurrentSubtitleSource(subtitleList.at(0));
    }
}

VideoBLLPtr VideoBLLLocal::operator+(const VideoBLLPtr v) {
    auto vl = dynamic_cast<const VideoBLLLocal *>(v.get());
    auto files = filePaths + (vl == nullptr ? QStringList() : vl->filePaths);
    return VideoBLLPtr(new VideoBLLLocal(files));
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
    if (sourceList.contains(getCurrentVideoSource())) {
        callAble(filePaths[sourceList.indexOf(getCurrentVideoSource())]);
    } else {
        callAble(Result<QString>());
    }
}

void VideoBLLLocal::loadVideoToPlay(QObject*, std::function<void(const Result<QString>&)> callAble) {
    loadVideoToPlay(callAble);
}

void VideoBLLLocal::loadThumbnail(std::function<void(const Result<QImage>&)> callAble) {
    if (sourceList.contains(getCurrentVideoSource())) {
        callAble(NekoAV::GetMediaFileIcon(filePaths[sourceList.indexOf(getCurrentVideoSource())]));
    } else {
        callAble(Result<QImage>());
    }
}

void VideoBLLLocal::loadThumbnail(QObject*, std::function<void(const Result<QImage>&)> callAble) {
    loadThumbnail(callAble);
}

void VideoBLLLocal::loadDanmaku(std::function<void(const Result<DanmakuList>&)> callAble) {
    if (!getCurrentDanmakuSource().isEmpty() && danmakuList.size() > 0) {
        // TODO(llhsdmd): 怎么导入本地弹幕给视频
    }
}

void VideoBLLLocal::loadDanmaku(QObject*, std::function<void(const Result<DanmakuList>&)> callAble) {
    loadDanmaku(callAble);
}

void VideoBLLLocal::loadSubtitle(std::function<void(const Result<QString>&)> callAble) {
    auto source = getCurrentSubtitleSource();
    if (!source.isEmpty() && source != "无") {
        callAble(source);
    }
}

void VideoBLLLocal::loadSubtitle(QObject*, std::function<void(const Result<QString>&)> callAble) {
    loadSubtitle(callAble);
}
