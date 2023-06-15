#include "videoBLL.hpp"

#include <QFileInfo>

VideoBLL::VideoBLL(const EpisodePtr episode) : video(episode) {

}

VideoBLLPtr VideoBLL::createVideoBLL(const QString filepath) {
    return VideoBLLPtr((VideoBLL*)new VideoBLLLocal(filepath));
}

VideoBLLPtr VideoBLL::createVideoBLL(const EpisodePtr episode) {
     return VideoBLLPtr(new VideoBLL(episode));
}

QListWidgetItem* VideoBLL::addToList(QListWidget* listWidget) {
    QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/loading_bar.png"), video->title());
    video->icon().then([item](const Result<QImage> &image){
        if (image.has_value()) {
            item->setIcon(QPixmap::fromImage(image.value()));
        } else {
            item->setIcon(QIcon());
        }
    });
    listWidget->addItem(item);
    return item;
}

QString VideoBLL::loadVideo() {
    return "";
}

QString VideoBLL::title() {
    return video->title();
}

VideoBLLLocal::VideoBLLLocal(const QString filepath) : filePath(filepath) {

}

QListWidgetItem* VideoBLLLocal::addToList(QListWidget* listWidget) {
    QListWidgetItem* item = new QListWidgetItem(QIcon(":/icons/loading_bar.png"), QFileInfo(filePath).fileName());
    listWidget->addItem(item);
    return item;
}

QString VideoBLLLocal::loadVideo() {
    return filePath;
}

QString VideoBLLLocal::title() {
    return QFileInfo(filePath).fileName();
}