#include "homeWidget.hpp"
#include "ui_homeView.h"

#include <QObjectList>
#include <QResizeEvent>
#include <QScrollBar>
#include <QMetaObject>

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

    _setupUi();

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value){
        if (value >= verticalScrollBar()->maximum()) {
            dataRequest(DisplayArea::Recommend);
        }
    });

    QMetaObject::invokeMethod(this, [this](){
            // 初始化所有区域请求数据更新
            refreshRequest(New);
            refreshRequest(Monday);
            refreshRequest(Tuesday);
            refreshRequest(Wednesday);
            refreshRequest(Thursday);
            refreshRequest(Friday);
            refreshRequest(Saturday);
            refreshRequest(Sunday);
            refreshRequest(Recommend);
        },
        Qt::ConnectionType::QueuedConnection);

    homeView->timeTab->setCurrentRow(0);
    homeView->recommend->installEventFilter(this);
}

void HomeWidget::refresh(const QList<videoData>& dataList,const DisplayArea area) {
    auto homeView = static_cast<Ui::homeView*>(ui);
    switch (area)
    {
    case DisplayArea::Recommend:
        _refresh(homeView->recommend, dataList);
        break;
    case DisplayArea::Monday:
        _refresh(homeView->mondayContents, dataList);
        break;
    case DisplayArea::Tuesday:
        _refresh(homeView->tuesdayContents, dataList);
        break;
    case DisplayArea::Wednesday:
        _refresh(homeView->wednesdayContents, dataList);
        break;
    case DisplayArea::Thursday:
        _refresh(homeView->thursdayContents, dataList);
        break;
    case DisplayArea::Friday:
        _refresh(homeView->fridayContents, dataList);
        break;
    case DisplayArea::Saturday:
        _refresh(homeView->saturdayContents, dataList);
        break;
    case DisplayArea::Sunday:
        _refresh(homeView->sundayContents, dataList);
        break;
    case DisplayArea::New:
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

void HomeWidget::_setupUi() {
    auto homeView = static_cast<Ui::homeView*>(ui);
    // 设置推荐区域为流式布局
    homeView->recommend->setLayout(new FlowLayout());

    // 设置主界面滑动条不显示
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 设置更新栏滑动条款式
    homeView->mondayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    homeView->mondayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    
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
}

QList<VideoView *> HomeWidget::_addItems(QWidget* container, int count) {
    QList<VideoView *> result;
    for (int i = 0;i < count; ++i) {
        auto videoView = new VideoView();
        container->layout()->addWidget(videoView);
        result.append(videoView);
    }
    QMetaObject::invokeMethod(this, [container, this](){
        QResizeEvent event(container->size(), container->size());
        eventFilter(container, &event);
    },Qt::QueuedConnection);
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
        auto width = homeView->recommend->size().width();
        auto preferred_height = homeView->recommend->layout()->heightForWidth(width); 
        preferred_height = std::max(preferred_height, 200);
        contents->setFixedHeight(preferred_height + homeView->recommend->geometry().y() + 3);
    }

    return QScrollArea::eventFilter(obj, event);
}

void HomeWidget::mousePressEvent(QMouseEvent *event) {
    // QScrollArea::mousePressEvent(event);
}

void HomeWidget::mouseMoveEvent(QMouseEvent *event) {
    // QScrollArea::mouseMoveEvent(event);
}

void HomeWidget::mouseReleaseEvent(QMouseEvent *event) {
    // QScrollArea::mouseReleaseEvent(event);
}

void HomeWidget::mouseDoubleClickEvent(QMouseEvent *event) {
    // QScrollArea::mouseDoubleClickEvent(event);
}

void HomeWidget::wheelEvent(QWheelEvent *event) {
    QScrollArea::wheelEvent(event);
}

bool HomeWidget::viewportEvent(QEvent *event) {

    return QScrollArea::viewportEvent(event);
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
    case DisplayArea::Recommend:
        return _addItems(homeView->recommend, count);
        break;
    case DisplayArea::Monday:
        return _addItems(homeView->mondayContents, count);
        break;
    case DisplayArea::Tuesday:
        return _addItems(homeView->tuesdayContents, count);
        break;
    case DisplayArea::Wednesday:
        return _addItems(homeView->wednesdayContents, count);
        break;
    case DisplayArea::Thursday:
        return _addItems(homeView->thursdayContents, count);
        break;
    case DisplayArea::Friday:
        return _addItems(homeView->fridayContents, count);
        break;
    case DisplayArea::Saturday:
        return _addItems(homeView->saturdayContents, count);
        break;
    case DisplayArea::Sunday:
        return _addItems(homeView->sundayContents, count);
        break;
    case DisplayArea::New:
        return _addItems(homeView->recentlyUpdatedContents, count);
        break;
    }
    return QList<VideoView *>();
}
