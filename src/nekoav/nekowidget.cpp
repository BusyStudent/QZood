#define NEKO_SOURCE
#include "nekoav.hpp"
#include "nekoprivate.hpp"
#include <QPainter>
#include <mutex>

namespace NekoAV {


VideoWidget::VideoWidget(QWidget *parent) : QWidget(parent), d(new VideoWidgetPrivate(this)) {
    // Setup signal / slot
    connect(videoSink(), &VideoSink::videoFrameChanged, d.get(), &VideoWidgetPrivate::_on_VideoFrameChanged, Qt::QueuedConnection);
}
VideoWidget::~VideoWidget() {

}
void VideoWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    if (d->image.isNull()) {
        return;
    }

    qreal texWidth = d->image.width();
    qreal texHeight = d->image.height();
    qreal winWidth = width();
    qreal winHeight = height();

    qreal x, y, w, h;

    if(texWidth * winHeight > texHeight * winWidth){
        w = winWidth;
        h = texHeight * winWidth / texWidth;
    }
    else{
        w = texWidth * winHeight / texHeight;
        h = winHeight;
    }
    x = (winWidth - w) / 2;
    y = (winHeight - h) / 2;

    // Size the item Size

    painter.drawImage(QRectF(x, y, w, h), d->image);
}
VideoSink *VideoWidget::videoSink() const {
    return &d->sink;
}

void VideoWidgetPrivate::_on_VideoFrameChanged(const VideoFrame &frame) {
    if (frame.isNull()) {
        return;
    }

    std::lock_guard locker(frame);

    Q_ASSERT(frame.pixelFormat() == VideoPixelFormat::RGBA32);

    int w = frame.width();
    int h = frame.height();
    int pitch = frame.bytesPerLine(0);
    uchar *pixels = frame.bits(0);
    if (image.isNull() || image.width() != w || image.height() != h) {
        image = QImage(w, h, QImage::Format_RGBA8888);
        image.fill(Qt::black);
    }
    uchar *dst = image.bits();
    int    dstPitch = image.bytesPerLine();

    // Update it
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            *((uint32_t*)   &dst[y * dstPitch + x * 4]) = *(
                (uint32_t*) &pixels[y * pitch + x * 4]
            );
        }
    }
    

    videoWidget->update();
}

GraphicsVideoItem::GraphicsVideoItem(QGraphicsObject *parent) : QGraphicsObject(parent), d(new GraphicsVideoItemPrivate(this)) {
    // Setup signal / slot
    connect(videoSink(), &VideoSink::videoFrameChanged, d.get(), &GraphicsVideoItemPrivate::_on_VideoFrameChanged, Qt::QueuedConnection);
}
GraphicsVideoItem::~GraphicsVideoItem() {

}
void GraphicsVideoItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {

    // if (d->image.isNull()) {
    //     return;
    // }

    // qreal texWidth = d->image.width();
    // qreal texHeight = d->image.height();
    // qreal winWidth = width();
    // qreal winHeight = height();

    // qreal x, y, w, h;

    // if(texWidth * winHeight > texHeight * winWidth){
    //     w = winWidth;
    //     h = texHeight * winWidth / texWidth;
    // }
    // else{
    //     w = texWidth * winHeight / texHeight;
    //     h = winHeight;
    // }
    // x = (winWidth - w) / 2;
    // y = (winHeight - h) / 2;

    // // Size the item Size

    // painter.drawImage(QRectF(x, y, w, h), d->image);
    auto bx = boundingRect();
    painter->drawImage(QRectF(pos(), d->size), d->image);
}
VideoSink *GraphicsVideoItem::videoSink() const {
    return &d->sink;
}
QSizeF     GraphicsVideoItem::nativeSize() const {
    return d->image.size();
}

void GraphicsVideoItemPrivate::_on_VideoFrameChanged(const VideoFrame &frame) {
    if (frame.isNull()) {
        return;
    }
    std::lock_guard locker(frame);

    Q_ASSERT(frame.pixelFormat() == VideoPixelFormat::RGBA32);

    int w = frame.width();
    int h = frame.height();
    int pitch = frame.bytesPerLine(0);
    uchar *pixels = frame.bits(0);
    if (image.isNull() || image.width() != w || image.height() != h) {
        image = QImage(w, h, QImage::Format_RGBA8888);
        image.fill(Qt::black);

        Q_EMIT videoItem->nativeSizeChanged(QSizeF(w, h));
    }
    uchar *dst = image.bits();
    int    dstPitch = image.bytesPerLine();
    
    // Update it
    for (int y = 0; y < image.height(); y++) {
        for (int x = 0; x < image.width(); x++) {
            *((uint32_t*)   &dst[y * dstPitch + x * 4]) = *(
                (uint32_t*) &pixels[y * pitch + x * 4]
            );
        }
    }

    videoItem->update();
}
QRectF GraphicsVideoItem::boundingRect() const {
    return 	QRectF(0, 0, d->image.width(), d->image.height());
}
void   GraphicsVideoItem::setSize(const QSizeF &size) {
    d->size = size;
}



}