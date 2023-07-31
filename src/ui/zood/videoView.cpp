#include "ui_videoView.h"
#include "videoView.hpp"
#include "../../common/myGlobalLog.hpp"

struct SizeStyle {
    int width = 185;
    int height = 160;
    int iconWidth = 185;
    int iconHeight = 100;
    int titleWidth = 185;
    int titleHeight = 19;
    int sourceWidth = 185;
    int sourceHeight = 19;
    int infoWidth = 185;
    int infoHeight = 19;
};

const SizeStyle kVideoSizeStyleH {185, 160, 185, 100, 185, 19, 185, 19, 185, 19};
const SizeStyle kVideoSizeStyleV {120, 200, 120, 140, 120, 19, 120, 19, 120, 19};

VideoView::VideoView(QWidget *parent, Direction direction) : QWidget(parent), ui(new Ui::VideoView()) {
    ui->setupUi(this);

    ui->videoIcon->installEventFilter(this);
    ui->videoTitle->installEventFilter(this);
    ui->videoExtraInfo->installEventFilter(this);
    ui->videoSource->installEventFilter(this);

    setDirection(direction);
}

void VideoView::setDirection(Direction direction) {
    auto setSizeStyle = [this, &direction](const SizeStyle sizeStyle) {
        setFixedSize(sizeStyle.width, sizeStyle.height);
        ui->videoIcon->setMinimumSize(sizeStyle.iconWidth, sizeStyle.iconHeight);
        ui->videoTitle->setMinimumSize(sizeStyle.titleWidth, sizeStyle.titleHeight);
        ui->videoSource->setMinimumSize(sizeStyle.sourceWidth, sizeStyle.sourceHeight);
        ui->videoExtraInfo->setMinimumSize(sizeStyle.infoWidth, sizeStyle.infoHeight);
    };

    if (mDirection != direction) {
        switch (direction) {
            case Direction::Vertical: setSizeStyle(kVideoSizeStyleV); break;
            case Direction::Horizontal: setSizeStyle(kVideoSizeStyleH); break;
        }
        mDirection = direction;
    }
}

VideoView::~VideoView() { }

void VideoView::setTitle(const QString &str, const QString &tooltip) {
    mTitle = str;
    QFontMetrics fontWidth(ui->videoTitle->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, width());
    ui->videoTitle->setText(elideNote);
    if (tooltip.isEmpty()) {
        ui->videoTitle->setToolTip(str);
    } else {
        ui->videoTitle->setToolTip(tooltip);
    }
}

void VideoView::setExtraInformation(const QString &str, const QString &tooltip) {
    mInfo = str;
    QFontMetrics fontWidth(ui->videoExtraInfo->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, width());
    ui->videoExtraInfo->setText(elideNote);
    if (tooltip.isEmpty()) {
        ui->videoExtraInfo->setToolTip(str);
    } else {
        ui->videoExtraInfo->setToolTip(tooltip);
    }
}

void VideoView::setSourceInformation(const QString &str, const QString &tooltip) {
    mSource = str;
    QFontMetrics fontWidth(ui->videoSource->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, width());
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

void VideoView::setVideoPtr(const RefPtr<DataObject> &videoPtr) {
    mVideoPtr = videoPtr;
}

bool VideoView::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::MouseButtonPress) {
        if (obj == ui->videoIcon) {
            Q_EMIT clickedImage(mVideoPtr);
        } else if (obj == ui->videoTitle) {
            Q_EMIT clickedTitle(mVideoPtr, ui->videoTitle->text());
        } else if (obj == ui->videoExtraInfo) {
            Q_EMIT clickedExtraInformation(mVideoPtr, ui->videoExtraInfo->text());
        } else if (obj == ui->videoSource) {
            Q_EMIT clickedSourceInformation(mVideoPtr, ui->videoSource->text());
        }
        Q_EMIT clicked(mVideoPtr);
    }

    return QWidget::eventFilter(obj, event);
}

QString VideoView::videoTitle() const {
    return ui->videoTitle->toolTip();
}

void VideoView::setTimelineEpisode(TimelineEpisodePtr tep) {
    setDirection(Vertical);
    setImage(kLoadingImage);
    setVideoPtr(tep);
    setTitle(tep->bangumiTitle());
    setExtraInformation(tep->pubIndexTitle());
    setSourceInformation(tep->availableSource().join(","));
    tep->fetchCover().then(this, [this](const Result<QImage>& img) {
        if (img.has_value()) {
            setImage(img.value());
        }
    });
}


RefPtr<DataObject> VideoView::videoPtr() const {
    return mVideoPtr;
}
