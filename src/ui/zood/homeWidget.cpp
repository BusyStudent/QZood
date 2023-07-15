#include "homeWidget.hpp"
#include "ui_homeView.h"

#include <QObjectList>
#include <QResizeEvent>
#include <QScrollBar>
#include <QMetaObject>

#include "../common/flowlayout.hpp"
#include "../player/playerWidget.hpp"
#include "../../BLL/data/videoSourceBLL.hpp"
#include "../../BLL/manager/videoDataManager.hpp"
#include "../../common/myGlobalLog.hpp"

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)

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

class HomeWidgetPrivate {
public:
    HomeWidgetPrivate(HomeWidget* self) : self(self), ui(new Ui::HomeView) { }
    ~HomeWidgetPrivate() { }

    void setupUi() {
        centralWidget = new QWidget();
        ui->setupUi(centralWidget);

        areaToWidget = {
            {HomeWidget::Monday, ui->mondayContents},
            {HomeWidget::Tuesday, ui->tuesdayContents},
            {HomeWidget::Wednesday, ui->wednesdayContents},
            {HomeWidget::Thursday, ui->thursdayContents},
            {HomeWidget::Friday, ui->fridayContents},
            {HomeWidget::Saturday, ui->saturdayContents},
            {HomeWidget::Sunday, ui->sundayContents},
            {HomeWidget::Recommend, ui->recommend},
            {HomeWidget::New, ui->recentlyUpdatedContents}
        };

        self->setWidget(centralWidget);
        self->setObjectName("HomeWidget");
        self->setStyleSheet(R"(QScrollArea#HomeWidget{
            border:none;
            background: transparent;
            })");
        QWidget::connect(self->verticalScrollBar(), &QScrollBar::valueChanged, self, [this](int value){
            if (value >= self->verticalScrollBar()->maximum()) {
                self->dataRequest();
            }
        });
        QWidget::connect(self, &HomeWidget::refreshRequest, VideoDataManager::instance(), &VideoDataManager::requestTimelineData);
        QWidget::connect(VideoDataManager::instance(), &VideoDataManager::timelineDataReply, self, [this](TimelineEpisodeList tep, int week) {
            auto w = (HomeWidget::DisplayArea)week;
            updateVideo(areaToWidget[w], tep);
        });

        QMetaObject::invokeMethod(self, [this](){
            // 初始化所有区域请求数据更新
            self->refreshRequest();
        }, Qt::ConnectionType::QueuedConnection);

        ui->timeTab->setCurrentRow(0);
        ui->recommend->installEventFilter(self);

        // 设置推荐区域为流式布局
        ui->recommend->setLayout(new FlowLayout());

        // 设置主界面滑动条不显示
        self->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        self->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        // 设置更新栏滑动条款式
        ui->mondayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->mondayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
        
        ui->tuesdayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->tuesdayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
        
        ui->wednesdayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->wednesdayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);

        ui->thursdayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->thursdayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
        
        ui->fridayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->fridayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
        
        ui->saturdayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->saturdayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
        
        ui->sundayContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->sundayContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
        
        ui->recentlyUpdatedContainer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        ui->recentlyUpdatedContainer->horizontalScrollBar()->setStyleSheet(kScrollBarStyleSheet);
    }

    void refresh(QWidget* container, const VideoDataVector &videoVector){
        clearItems(container);
        auto videos = addItems(container, videoVector.size());
        for (int i = 0;i < videoVector.size(); ++i) {
            videos[i]->setImage(videoVector[i].image);
            videos[i]->setTitle(videoVector[i].videoTitle);
            videos[i]->setExtraInformation(videoVector[i].videoExtraInformation);
            videos[i]->setSourceInformation(videoVector[i].videoSourceInformation);
        }
    }
    void updateVideo(QWidget* container, TimelineEpisodeList teps) {
        auto videos = container->findChildren<VideoView*>();
        for (auto tep : teps) {
            bool f = false;
            for (auto video : videos) {
                if (video->videoId() == tep->bangumiTitle()) {
                    video->setTimelineEpisode(tep);
                    f = true;
                    break;
                }
            }
            if (!f) {
                auto video = addItems(container, 1).at(0);
                video->setTimelineEpisode(tep);
            }
        }
    }
    void updateVideo(QWidget* container, const VideoDataVector &videoVector) {
        auto videos = container->findChildren<VideoView*>();
        for (int i = 0;i < videoVector.size(); ++i) {
            auto f = false;
            for (auto video : videos) {
                if (video->videoId() == videoVector[i].videoId) {
                    video->setImage(videoVector[i].image);
                    video->setTitle(videoVector[i].videoTitle);
                    video->setExtraInformation(videoVector[i].videoExtraInformation);
                    video->setSourceInformation(videoVector[i].videoSourceInformation);
                    f = true;
                    break;
                }
            }
            if (!f) {
                auto video = addItems(container, 1).at(0);
                video->setVideoId(videoVector[i].videoId);
                video->setImage(videoVector[i].image);
                video->setTitle(videoVector[i].videoTitle);
                video->setExtraInformation(videoVector[i].videoExtraInformation);
                video->setSourceInformation(videoVector[i].videoSourceInformation);
            }
        }
    }
    
    QList<VideoView* > addItems(QWidget* container, int count) {
        QList<VideoView *> result;
        assert(container != nullptr);
        for (int i = 0;i < count; ++i) {
            auto videoView = new VideoView();
            container->layout()->addWidget(videoView);
            result.append(videoView);
            QWidget::connect(videoView, &VideoView::clicked, self, [this](const QString &Id){
                auto videoSource = VideoSourceBLL::instance();
                MDebug(MyDebug::WARNING) << "open video " << Id;
                QApplication::setOverrideCursor(Qt::WaitCursor);
                videoSource->searchVideosFromTitle(Id, self, [this, Id](const Result<VideoBLLList>& videos){
                    if (videos.has_value()){
                        self->runPlayer(videos.value(), Id);
                    } else {
                        MDebug(MyDebug::WARNING) << "video " << Id << " not has data!";
                    }
                    QApplication::restoreOverrideCursor();
                });
            });
        }
        QMetaObject::invokeMethod(self, [container, this](){
            QResizeEvent event(container->size(), container->size());
            self->eventFilter(container, &event);
        },Qt::QueuedConnection);
        return result;
    }
    
    void clearItems(QWidget* container){
        if (nullptr == container) {
        return;
        }
        for (auto item : container->findChildren<QWidget*>()) {
            auto wi = dynamic_cast<QWidget*>(item);
            if (nullptr != wi) {
                wi->setParent(nullptr);
                wi->deleteLater();
            }
        }
    }

public:
    QScopedPointer<Ui::HomeView> ui;
    QWidget* centralWidget;
    QMap<HomeWidget::DisplayArea, QWidget*> areaToWidget;


private:
    HomeWidget* self;
};

HomeWidget::HomeWidget(QWidget* parent) : QScrollArea(parent), d(new HomeWidgetPrivate(this)) {
    d->setupUi();
}

HomeWidget::~HomeWidget() { }

void HomeWidget::refresh(const VideoDataVector& dataVector,const DisplayArea area) {
    d->refresh(d->areaToWidget[area], dataVector);
}

void HomeWidget::updateVideo(const VideoDataVector& dataVector,const DisplayArea area) {
    d->updateVideo(d->areaToWidget[area], dataVector);
}

void HomeWidget::resizeEvent(QResizeEvent *event) {
    d->centralWidget->setFixedWidth(event->size().width());
    if (d->centralWidget->height() < event->size().height()) {
        d->centralWidget->setMinimumHeight(event->size().height());
    }
    QScrollArea::resizeEvent(event);
}

bool HomeWidget::eventFilter(QObject *obj,QEvent *event) {
    if (obj == d->ui->recommend && event->type() == QEvent::Type::Resize) {
        auto width = d->ui->recommend->size().width();
        auto preferred_height = d->ui->recommend->layout()->heightForWidth(width); 
        preferred_height = max(preferred_height, 200);
        d->centralWidget->setFixedHeight(preferred_height + d->ui->recommend->geometry().y() + 3);
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


VideoView* HomeWidget::addItem(const DisplayArea& area) {
    auto result = addItems(area, 1);
    if (result.size() == 0) {
        return nullptr;
    }
    return result.at(0);
}

void HomeWidget::clearItem(const DisplayArea& area) {
    return d->clearItems(d->areaToWidget[area]);
}

QList<VideoView *> HomeWidget::addItems(const DisplayArea& area, int count) {
    return d->addItems(d->areaToWidget[area], count);
}

void HomeWidget::runPlayer(VideoBLLList videos,const QString &bangumiName) {
    PlayerWidget* player = new PlayerWidget();
    player->setTitle(bangumiName);
    player->setAttribute(Qt::WA_DeleteOnClose);
    player->setVideoList(videos);
    player->show();
}