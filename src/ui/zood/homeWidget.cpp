#include "homeWidget.hpp"
#include "ui_homeView.h"
#include "ui_videoView.h"

#include <QObjectList>
#include <QResizeEvent>
#include <QScrollBar>

#include "../common/flowlayout.hpp"

const QString kScrollBarStyleSheet =
R"(
QScrollBar:horizontal {
    height: 10px;
    border: 1px solid grey;
    border-radius: 5px;
    background: #ffffff;
    margin: 0px 0px 0px 0px;
}
QScrollBar::handle:horizontal {
    background: #696969;
    min-width: 20px;
}
QScrollBar::add-line:horizontal {
    width: 0px;
}
QScrollBar::sub-line:horizontal {
    width: 0px;
}
)";

HomeWidget::HomeWidget(QWidget* parent) : QScrollArea(parent), ui(new Ui::homeView()) {
    contents = new QWidget();
    auto homeView = static_cast<Ui::homeView*>(ui);
    homeView->setupUi(contents);

    setWidget(contents);
    setObjectName("homeWidget");
    
    homeView->recommend->setLayout(new FlowLayout());

    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->mondayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->mondayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    // homeView->mondayContainer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->tuesdayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->tuesdayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    homeView->wednesdayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->wednesdayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    homeView->thursdayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->thursdayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    homeView->fridayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->fridayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    homeView->saturdayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->saturdayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    homeView->sundayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->sundayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    homeView->recentlyUpdatedContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->recentlyUpdatedContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);



    refreshRequest(TimeNew);
    refreshRequest(TimeMonday);
    refreshRequest(TimeTuesday);
    refreshRequest(TimeWednesday);
    refreshRequest(TimeThursday);
    refreshRequest(TimeFriday);
    refreshRequest(TimeSaturday);
    refreshRequest(TimeSunday);
    refreshRequest(TimeRecommend);

    addItems(TimeMonday, 10);
    addItems(TimeRecommend, 10);

    homeView->timeTab->setCurrentRow(0);
    homeView->recommend->installEventFilter(this);
}

void HomeWidget::refresh(const QList<videoData>& dataList,const DisplayArea area) {
    auto homeView = static_cast<Ui::homeView*>(ui);
    switch (area)
    {
    case DisplayArea::TimeRecommend:
        _refresh(homeView->recommend, dataList);
        break;
    case DisplayArea::TimeMonday:
        _refresh(homeView->mondayContents, dataList);
        break;
    case DisplayArea::TimeTuesday:
        _refresh(homeView->tuesdayContents, dataList);
        break;
    case DisplayArea::TimeWednesday:
        _refresh(homeView->wednesdayContents, dataList);
        break;
    case DisplayArea::TimeThursday:
        _refresh(homeView->thursdayContents, dataList);
        break;
    case DisplayArea::TimeFriday:
        _refresh(homeView->fridayContents, dataList);
        break;
    case DisplayArea::TimeSaturday:
        _refresh(homeView->saturdayContents, dataList);
        break;
    case DisplayArea::TimeSunday:
        _refresh(homeView->sundayContents, dataList);
        break;
    case DisplayArea::TimeNew:
        _refresh(homeView->recentlyUpdatedContents, dataList);
        break;
    }
}

void HomeWidget::_refresh(QWidget* container, const QList<videoData>& dataList) {
    auto childs = container->children();
    for (auto &child : childs) {
        if (child->isWidgetType()) {
            child->deleteLater();
        }
    }
    auto videos = _addItems(container, dataList.size());
    for (int i = 0;i < dataList.size(); ++i) {
        videos[i]->setImage(dataList[i].image);
        videos[i]->setTitle(dataList[i].videoTitle);
        videos[i]->setExtraInformation(dataList[i].videoExtraInformation);
        videos[i]->setSourceInformation(dataList[i].videoSourceInformation);
    }
}

QList<VideoView *> HomeWidget::_addItems(QWidget* container, int count) {
    QList<VideoView *> result;
    for (int i = 0;i < count; ++i) {
        auto videoView = new VideoView();
        container->layout()->addWidget(videoView);
        result.append(videoView);
    }
    return result;
}

void HomeWidget::resizeEvent(QResizeEvent *event) {
    contents->setFixedWidth(event->size().width());
    if (contents->height() < event->size().height()) {
        contents->setMinimumHeight(event->size().height());
    }
    QScrollArea::resizeEvent(event);
}

bool HomeWidget::eventFilter(QObject *obj,QEvent *event) {
    auto homeView = static_cast<Ui::homeView*>(ui);
    if (obj == homeView->recommend && event->type() == QEvent::Type::Resize) {
        auto size_event = static_cast<QResizeEvent *>(event);
        qWarning() << "recommend : " << homeView->recommend->geometry();
        qWarning() << "size : " << homeView->recommend->size();
        qWarning() << "old size : " << size_event->oldSize();
        qWarning() << "new size : " << size_event->size();
        qWarning() << "layout size : " << homeView->recommend->layout()->totalSizeHint();
        contents->setMinimumHeight(homeView->recommend->layout()->totalSizeHint().height() / 2);
    }

    return QScrollArea::eventFilter(obj, event);
}

VideoView* HomeWidget::addItem(const DisplayArea& area) {
    auto result = addItems(area, 1);
    if (result.size() == 0) {
        return nullptr;
    }
    return result.at(0);
}

QList<VideoView *> HomeWidget::addItems(const DisplayArea& area, int count) {
    auto homeView = static_cast<Ui::homeView*>(ui);
    switch (area)
    {
    case DisplayArea::TimeRecommend:
        return _addItems(homeView->recommend, count);
        break;
    case DisplayArea::TimeMonday:
        return _addItems(homeView->mondayContents, count);
        break;
    case DisplayArea::TimeTuesday:
        return _addItems(homeView->tuesdayContents, count);
        break;
    case DisplayArea::TimeWednesday:
        return _addItems(homeView->wednesdayContents, count);
        break;
    case DisplayArea::TimeThursday:
        return _addItems(homeView->thursdayContents, count);
        break;
    case DisplayArea::TimeFriday:
        return _addItems(homeView->fridayContents, count);
        break;
    case DisplayArea::TimeSaturday:
        return _addItems(homeView->saturdayContents, count);
        break;
    case DisplayArea::TimeSunday:
        return _addItems(homeView->sundayContents, count);
        break;
    case DisplayArea::TimeNew:
        return _addItems(homeView->recentlyUpdatedContents, count);
        break;
    }
    return QList<VideoView *>();
}

void VideoView::setImage(const QImage& image) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    videoView->videoIcon->setPixmap(QPixmap::fromImage(image));
}
void VideoView::setTitle(const QString& str) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    QFontMetrics fontWidth(videoView->videoTitle->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, videoView->videoTitle->size().width());
    videoView->videoTitle->setText(elideNote);
}
void VideoView::setExtraInformation(const QString& str) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    QFontMetrics fontWidth(videoView->videMessage->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, videoView->videMessage->size().width());
    videoView->videMessage->setText(elideNote);
}
void VideoView::setSourceInformation(const QString& str) {
    auto videoView = static_cast<Ui::videoView*>(ui);
    QFontMetrics fontWidth(videoView->videSource->font());
    QString elideNote = fontWidth.elidedText(str, Qt::ElideRight, videoView->videSource->size().width());
    videoView->videSource->setText(elideNote);
}



VideoView::VideoView(QWidget* parent) : QWidget(parent), ui(new Ui::videoView()) {
    auto homeView = static_cast<Ui::videoView*>(ui);
    homeView->setupUi(this);
}