#include "homeWidget.hpp"
#include "ui_homeView.h"

#include <QObjectList>
#include <QResizeEvent>
#include <QScrollBar>
#include <QMetaObject>

#include "../common/flowlayout.hpp"
#include "../player/playerWidget.hpp"
#include "../../BLL/data/videoSourceBLL.hpp"

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
    ~HomeWidgetPrivate() {
        delete ui;
    }

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

    void refresh(QWidget* container, const QList<videoData>& dataList){
        auto childs = container->children();
        for (auto &child : childs) {
            if (child->isWidgetType()) {
                child->deleteLater();
            }
        }
        auto videos = addItems(container, dataList.size());
        for (int i = 0;i < dataList.size(); ++i) {
            videos[i]->setImage(dataList[i].image);
            videos[i]->setTitle(dataList[i].videoTitle);
            videos[i]->setExtraInformation(dataList[i].videoExtraInformation);
            videos[i]->setSourceInformation(dataList[i].videoSourceInformation);
        }
    }
    
    QList<VideoView* > addItems(QWidget* container, int count) {
        QList<VideoView *> result;
        assert(container != nullptr);
        for (int i = 0;i < count; ++i) {
            auto videoView = new VideoView();
            container->layout()->addWidget(videoView);
            result.append(videoView);
            QWidget::connect(videoView, &VideoView::clickedImage, self, [this](QString Id){
                auto videoSource = VideoSourceBLL::instance();
                QApplication::setOverrideCursor(Qt::WaitCursor);
                videoSource->searchVideosFromTitle(Id, self, [this](const Result<VideoBLLList>& videos){
                    if (videos.has_value()){
                        qDebug() << "player videos";
                        qDebug() << videos.value().size();
                        self->runPlayer(videos.value());
                    }
                    QApplication::restoreOverrideCursor();
                });
            });
            QWidget::connect(videoView, &VideoView::clickedTitle, self, [videoView](const QString &id, const QString &title){
                Q_EMIT videoView->clickedImage(id);
            });
            QWidget::connect(videoView, &VideoView::clickedSourceInformation, self, [videoView](const QString &id, const QString &sourceInfo){
                Q_EMIT videoView->clickedImage(id);
            });
            QWidget::connect(videoView, &VideoView::clickedExtraInformation, self, [videoView](const QString &id, const QString &extraInfo){
                Q_EMIT videoView->clickedImage(id);
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
    Ui::HomeView* ui;
    QWidget* centralWidget;
    QMap<HomeWidget::DisplayArea, QWidget*> areaToWidget;


private:
    HomeWidget* self;
};

HomeWidget::HomeWidget(QWidget* parent) : QScrollArea(parent), d(new HomeWidgetPrivate(this)) {
    d->setupUi();
}

HomeWidget::~HomeWidget() {
    delete d;
}

void HomeWidget::refresh(const QList<videoData>& dataList,const DisplayArea area) {
    d->refresh(d->areaToWidget[area], dataList);
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
        preferred_height = std::max(preferred_height, 200);
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

void HomeWidget::runPlayer(VideoBLLList videos) {
    PlayerWidget* player = new PlayerWidget();
    player->setAttribute(Qt::WA_DeleteOnClose);
    player->setVideoList(videos);
    player->show();
}