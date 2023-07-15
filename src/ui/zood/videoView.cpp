#include "ui_videoView.h"
#include "videoView.hpp"

VideoView::VideoView(QWidget *parent) : QWidget(parent), ui(new Ui::VideoView()) {
    ui->setupUi(this);

    ui->videoIcon->installEventFilter(this);
    ui->videoTitle->installEventFilter(this);
    ui->videoExtraInfo->installEventFilter(this);
    ui->videoSource->installEventFilter(this);
}

VideoView::~VideoView() { }

void VideoView::setTitle(const QString &str, const QString &tooltip) {
    QFontMetrics fontWidth(ui->videoTitle->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, width() - 10);
    ui->videoTitle->setText(elideNote);
    if (tooltip.isEmpty()) {
        ui->videoTitle->setToolTip(str);
    } else {
        ui->videoTitle->setToolTip(tooltip);
    }
}

void VideoView::setExtraInformation(const QString &str, const QString &tooltip) {
    QFontMetrics fontWidth(ui->videoExtraInfo->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, width() - 10);
    ui->videoExtraInfo->setText(elideNote);
    if (tooltip.isEmpty()) {
        ui->videoExtraInfo->setToolTip(str);
    } else {
        ui->videoExtraInfo->setToolTip(tooltip);
    }
}

void VideoView::setSourceInformation(const QString &str, const QString &tooltip) {
    QFontMetrics fontWidth(ui->videoSource->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, width() -  10);
    ui->videoSource->setText(elideNote);
    if (tooltip.isEmpty()) {
        ui->videoSource->setToolTip(str);
    } else {
        ui->videoSource->setToolTip(tooltip);
    }
}

void VideoView::setImage(const QImage &image, const QString &tooltip) {
    ui->videoIcon->setPixmap(QPixmap::fromImage(image.scaled(ui->videoIcon->size(), Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation)));
    ui->videoIcon->setAlignment(Qt::AlignCenter);
    ui->videoIcon->setToolTip(tooltip);
}

void VideoView::setVideoId(const QString &videoId) {
    mVideoId = videoId;
}

bool VideoView::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == ui->videoIcon) {
            Q_EMIT clickedImage(mVideoId);
        } else if (obj == ui->videoTitle) {
            Q_EMIT clickedTitle(mVideoId, ui->videoTitle->text());
        } else if (obj == ui->videoExtraInfo) {
            Q_EMIT clickedExtraInformation(mVideoId, ui->videoExtraInfo->text());
        } else if (obj == ui->videoSource) {
            Q_EMIT clickedSourceInformation(mVideoId, ui->videoSource->text());
        }
        Q_EMIT clicked(mVideoId);
    }

    return QWidget::eventFilter(obj, event);
}

void VideoView::setTimelineEpisode(TimelineEpisodePtr tep) {
    setImage(kLoadingImage);
    setVideoId(tep->bangumiTitle());
    setTitle(tep->bangumiTitle());
    setExtraInformation(tep->pubIndexTitle());
    setSourceInformation(tep->availableSource().join(","));
    tep->fetchCover().then(this, [this](const Result<QImage>& img) {
        if (img.has_value()) {
            setImage(img.value());
        }
    });
}


QString VideoView::videoId() const {
    return mVideoId;
}
