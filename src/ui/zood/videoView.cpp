
#include "ui_videoView.h"
#include "videoView.hpp"

VideoView::VideoView(QWidget* parent) : QWidget(parent), ui(new Ui::videoView()) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    videoView->setupUi(this);

    videoView->videoIcon->installEventFilter(this);
    videoView->videoTitle->installEventFilter(this);
    videoView->videoExtraInfo->installEventFilter(this);
    videoView->videoSource->installEventFilter(this);
}

void VideoView::setTitle(const QString& str, const QString& tooltip) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    QFontMetrics fontWidth(videoView->videoTitle->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, 185);
    videoView->videoTitle->setText(elideNote);
    if (tooltip.isEmpty()) {
        videoView->videoTitle->setToolTip(str);
    } else {
        videoView->videoTitle->setToolTip(tooltip);
    }
}

void VideoView::setExtraInformation(const QString& str, const QString& tooltip) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    QFontMetrics fontWidth(videoView->videoExtraInfo->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, 185);
    videoView->videoExtraInfo->setText(elideNote);
    if (tooltip.isEmpty()) {
        videoView->videoExtraInfo->setToolTip(str);
    } else {
        videoView->videoExtraInfo->setToolTip(tooltip);
    }
}

void VideoView::setSourceInformation(const QString& str, const QString& tooltip) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    QFontMetrics fontWidth(videoView->videoSource->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, 185);
    videoView->videoSource->setText(elideNote);
    if (tooltip.isEmpty()) {
        videoView->videoSource->setToolTip(str);
    } else {
        videoView->videoSource->setToolTip(tooltip);
    }
}

void VideoView::setImage(const QImage& image, const QString& tooltip) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    videoView->videoIcon->setPixmap(QPixmap::fromImage(image.scaled({185,100}, Qt::KeepAspectRatio)));
    videoView->videoIcon->setAlignment(Qt::AlignCenter);
    videoView->videoIcon->setToolTip(tooltip);
}

void VideoView::setVideoId(const int videoId) {
    this->videoId = videoId;
}

bool VideoView::eventFilter(QObject* obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        auto videoView = static_cast<Ui::videoView*>(ui);
        if (obj == videoView->videoIcon) {
            clickedImage(videoId);
        } else if (obj == videoView->videoTitle) {
            clickedTitle(videoId, videoView->videoTitle->text());
        } else if (obj == videoView->videoExtraInfo) {
            clickedExtraInformation(videoId, videoView->videoExtraInfo->text());
        } else if (obj == videoView->videoSource) {
            clickedSourceInformation(videoId, videoView->videoSource->text());
        }
    }

    return QWidget::eventFilter(obj, event);
}