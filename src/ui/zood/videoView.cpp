
#include "ui_videoView.h"
#include "videoView.hpp"

VideoView::VideoView(QWidget* parent) : QWidget(parent), ui(new Ui::VideoView()) {
    ui->setupUi(this);

    ui->videoIcon->installEventFilter(this);
    ui->videoTitle->installEventFilter(this);
    ui->videoExtraInfo->installEventFilter(this);
    ui->videoSource->installEventFilter(this);
}

void VideoView::setTitle(const QString& str, const QString& tooltip) {
    QFontMetrics fontWidth(ui->videoTitle->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, 185);
    ui->videoTitle->setText(elideNote);
    if (tooltip.isEmpty()) {
        ui->videoTitle->setToolTip(str);
    } else {
        ui->videoTitle->setToolTip(tooltip);
    }
}

void VideoView::setExtraInformation(const QString& str, const QString& tooltip) {
    QFontMetrics fontWidth(ui->videoExtraInfo->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, 185);
    ui->videoExtraInfo->setText(elideNote);
    if (tooltip.isEmpty()) {
        ui->videoExtraInfo->setToolTip(str);
    } else {
        ui->videoExtraInfo->setToolTip(tooltip);
    }
}

void VideoView::setSourceInformation(const QString& str, const QString& tooltip) {
    QFontMetrics fontWidth(ui->videoSource->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, 185);
    ui->videoSource->setText(elideNote);
    if (tooltip.isEmpty()) {
        ui->videoSource->setToolTip(str);
    } else {
        ui->videoSource->setToolTip(tooltip);
    }
}

void VideoView::setImage(const QImage& image, const QString& tooltip) {
    ui->videoIcon->setPixmap(QPixmap::fromImage(image.scaled({185,100}, Qt::KeepAspectRatio)));
    ui->videoIcon->setAlignment(Qt::AlignCenter);
    ui->videoIcon->setToolTip(tooltip);
}

void VideoView::setVideoId(const QString videoId) {
    this->videoId = videoId;
}

bool VideoView::eventFilter(QObject* obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == ui->videoIcon) {
            clickedImage(videoId);
        } else if (obj == ui->videoTitle) {
            clickedTitle(videoId, ui->videoTitle->text());
        } else if (obj == ui->videoExtraInfo) {
            clickedExtraInformation(videoId, ui->videoExtraInfo->text());
        } else if (obj == ui->videoSource) {
            clickedSourceInformation(videoId, ui->videoSource->text());
        }
    }

    return QWidget::eventFilter(obj, event);
}