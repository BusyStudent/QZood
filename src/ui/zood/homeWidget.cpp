#include "homeWidget.hpp"
#include "ui_homeView.h"

#include <QObjectList>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QScrollBar>
#include <QMetaObject>
#include <QTime>

#include "../util/layout/flowlayout.hpp"
#include "../player/playerWidget.hpp"
#include "../../BLL/data/videoSourceBLL.hpp"
#include "../../BLL/manager/videoDataManager.hpp"
#include "../../common/myGlobalLog.hpp"

#undef min
#undef max
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

        initializeAreaToWidget();

        self->setWidget(centralWidget);
        self->setObjectName("HomeWidget");
        QWidget::connect(self->verticalScrollBar(), &QScrollBar::valueChanged, self, [this](int value){
            if (value >= self->verticalScrollBar()->maximum()) {
                self->dataRequest();
            }
        });
        connectToDataLayer();

        requestDataLater();

        ui->timeTab->setCurrentRow(0);
        ui->timeAnimeView->installEventFilter(self);
        ui->recommend->installEventFilter(self);

        // 设置推荐区域为流式布局
        ui->recommend->setLayout(new FlowLayout());

        setupScrollBarStyle();
    }

    void requestDataLater() {
        QMetaObject::invokeMethod(self, [this]() {
            // 初始化所有区域请求数据更新
            self->refreshRequest();
            }, Qt::ConnectionType::QueuedConnection);
    }

    void connectToDataLayer() {
        QWidget::connect(self, &HomeWidget::refreshRequest, VideoDataManager::instance(), &VideoDataManager::requestTimelineData);
        QWidget::connect(VideoDataManager::instance(), &VideoDataManager::timelineDataReply, self, [this](TimelineEpisodeList tep, int week) {
            auto w = (HomeWidget::DisplayArea)week;
            QTime time;
            auto t1 = time.currentTime();
            updateVideo(areaToWidget[w], tep);
            auto t2 = time.currentTime();
        });
    }

    void initializeAreaToWidget() {
        areaToWidget = {
            { HomeWidget::Monday, ui->mondayContents },
            { HomeWidget::Tuesday, ui->tuesdayContents },
            { HomeWidget::Wednesday, ui->wednesdayContents },
            { HomeWidget::Thursday, ui->thursdayContents },
            { HomeWidget::Friday, ui->fridayContents },
            { HomeWidget::Saturday, ui->saturdayContents },
            { HomeWidget::Sunday, ui->sundayContents },
            { HomeWidget::Recommend, ui->recommend },
            { HomeWidget::New, ui->recentlyUpdatedContents }
        };
    }

    void setupScrollBarStyle() {
        self->setStyleSheet(R"(QScrollArea#HomeWidget{
            border:none;
            background: transparent;
            })");

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

    void updateVideo(QWidget* container, TimelineEpisodeList teps) {
        auto videos = container->findChildren<VideoView*>();
        for (auto tep : teps) {
            bool f = false;
            for (auto video : videos) {
                auto ptr = dynamic_cast<TimelineEpisode *>(video->videoPtr().get());
                if (nullptr != ptr && ptr->bangumiTitle() == tep->bangumiTitle()) {
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
    
    QList<VideoView* > addItems(QWidget* container, int count) {
        QList<VideoView *> result;
        assert(container != nullptr);
        for (int i = 0;i < count; ++i) {
            auto videoView = new VideoView();
            container->layout()->addWidget(videoView);
            result.append(videoView);
            QWidget::connect(videoView, &VideoView::clicked, self, [this, videoView](const RefPtr<DataObject> &voidePtr){
                auto videoSource = VideoSourceBLL::instance();
                MDebug(MyDebug::WARNING) << "open video " << videoView->videoTitle();
                QApplication::setOverrideCursor(Qt::WaitCursor);
                videoSource->searchVideosFromBangumi(voidePtr, self, [this, videoView](const Result<VideoBLLList>& videos){
                    if (videos.has_value()){
                        self->runPlayer(videos.value(), videoView->videoTitle());
                    } else {
                        MDebug(MyDebug::WARNING) << "video " << videoView->videoTitle() << " not has data!";
                    }
                    QApplication::restoreOverrideCursor();
                });
            });
        }
        QMetaObject::invokeMethod(self, [container, this](){
            QResizeEvent event(container->size(), container->size());
            self->eventFilter(container, &event);
        }, Qt::QueuedConnection);
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
    } else if (obj == d->ui->timeAnimeView && event->type() == QEvent::Type::Wheel) {
        auto wheel_event = dynamic_cast<QWheelEvent*>(event);
        auto wi = d->ui->timeAnimeView->currentWidget()->findChild<QScrollArea*>();
        wi->horizontalScrollBar()->setValue(wi->horizontalScrollBar()->value() - wheel_event->angleDelta().y());
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