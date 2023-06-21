#include "playerWidget.hpp"
#include "ui_playerView.h"
#include "ui_playOrderSettingView.h"

#include <QMenuBar>
#include <QMouseEvent>
#include <climits>
#include <QMimeData>
#include <QTime>
#include <QWindow>
#include <QFileInfo>
#include <QFileDialog>

#include "videoWidget.hpp"
#include "../common/popupWidget.hpp"
#include "../../BLL/data/videoItemModel.hpp"

class PlayerWidgetPrivate {
public:
    PlayerWidgetPrivate(PlayerWidget* parent) : self(parent), ui(new Ui::PlayerView()) {
        itemModel = new VideoItemModel();
    }

    ~PlayerWidgetPrivate() {
        delete ui;
        delete itemModel;
    }

    void setupUi() {
        ui->setupUi(self);
        ui->playlist->setFocusPolicy(Qt::FocusPolicy::ClickFocus);

        // 设置默认划分比例
        ui->splitter->setStretchFactor(0, 10);
        ui->splitter->setStretchFactor(1, 1);

        // 实例化视频播放核心控件
        videoWidget = new VideoWidget(ui->videoPlayContainer);
        ui->videoPlayContainer->layout()->addWidget(videoWidget);

        // 播放顺序设置界面
        playOrderSettingWidget = new PopupWidget(self);
        ui_playOrderSettingView = new Ui::PlayOrderSettingView();
        ui_playOrderSettingView->setupUi(playOrderSettingWidget);
        playOrderSettingWidget->setAssociateWidget(ui->playOrder);
        playOrderSettingWidget->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        playOrderSettingWidget->setAuotLayout();
    }

    void update() {
        // TODO(llhsdmd): 使用数据模型更新界面ui
        
    }

    void connect() {
        connectTitleBar();
        connectPlayList();
    }

    void addVideo(const VideoBLLPtr video) {
        itemModel->add(video);
        video->addToList(ui->playlist);
    }

private:
    /**
     * @brief 标题栏
     */
    void connectTitleBar() {
        // 设置窗口置顶功能
        QWidget::connect(ui->onTopButton, &QToolButton::clicked, self, [this, flags = self->windowFlags()]() mutable {
            QWindow* pWin = self->windowHandle();
            if (ui->onTopButton->isChecked()) {
                flags = self->windowFlags();
                pWin->setFlags(flags | Qt::WindowStaysOnTopHint);
            } else {
                pWin->setFlags(flags);
            }
        });
        QWidget::connect(ui->minimizeButton, &QToolButton::clicked, self, [this](){
            self->showMinimized();
        });
        QWidget::connect(ui->maximizeButton, &QToolButton::clicked, self, [this](){
            if (self->isMaximized()) {
                self->showNormal();
            } else {
                self->showMaximized();
            }
        });
        QWidget::connect(ui->closeButton, &QToolButton::clicked, self, [this](){
            videoWidget->stop();
            self->close();
        });
        QWidget::connect(ui->miniPlayerButton, &QToolButton::clicked, self, [this]() mutable {
            // TODO(llhsdmd): 实现小窗播放请求
        });
    }

    /**
     * @brief 视频播放列表设置
     */
    void connectPlayList() {
        // 切换播放列表的视图模式
        QWidget::connect(ui->listViewMode, &QToolButton::clicked, self, [this](bool clicked){
            switch (ui->playlist->viewMode()) {
                case QListView::ViewMode::IconMode:
                    ui->playlist->setViewMode(QListView::ViewMode::ListMode);
                    ui->listViewMode->setIcon(QIcon(":/icons/list_text_white.png"));
                    break;
                case QListView::ViewMode::ListMode:
                    ui->playlist->setViewMode(QListView::ViewMode::IconMode);
                    ui->listViewMode->setIcon(QIcon(":/icons/list_icon_white.png"));
                    break;
            }
        });
        // 更改播放顺序设置。
        QWidget::connect(ui->playOrder, &QToolButton::clicked, self, [this](bool clicked){
            playOrderSettingWidget->show();
            playOrderSettingWidget->hideLater(5000);
        });
        QWidget::connect(ui_playOrderSettingView->inOrderButton, &QToolButton::clicked, self, [this]() {
            setPlayOrder(Order::IN_ORDER);
            ui->playOrder->setIcon(QIcon(":/icons/order_down_white.png"));
        });
        QWidget::connect(ui_playOrderSettingView->listLoopButton, &QToolButton::clicked, self, [this]() {
            setPlayOrder(Order::LIST_LOOP);
            ui->playOrder->setIcon(QIcon(":/icons/repeat_list_white.png"));
        });
        QWidget::connect(ui_playOrderSettingView->listRandomButton, &QToolButton::clicked, self, [this]() {
            setPlayOrder(Order::LIST_RANDOM);
            ui->playOrder->setIcon(QIcon(":/icons/random_list_white.png"));
        });
        QWidget::connect(ui_playOrderSettingView->singleCycleButton, &QToolButton::clicked, self, [this]() {
            setPlayOrder(Order::SINGLE_CYCLE);
            ui->playOrder->setIcon(QIcon(":/icons/repeat_one_white.png"));
        });
        QWidget::connect(ui_playOrderSettingView->stopButton, &QToolButton::clicked, self, [this]() {
            setPlayOrder(Order::STOP);
            ui->playOrder->setIcon(QIcon(":/icons/order_one_white.png"));
        });
        QWidget::connect(ui->addVideo, &QToolButton::clicked, self, [this]() {
            // TODO(llhsdmd): 添加视频，暂时只能添加本地视频到播放列表算了。
            auto filePaths = QFileDialog::getOpenFileNames(self, self->tr("文件"), "./", "*.mp4;*.mkv;;*.*");
            for (auto filePath : filePaths) {
                if (!filePath.isEmpty()) {
                    addVideo(createVideoBLL(filePath));
                }
            }
        });
        QWidget::connect(ui->playlist, &QListWidget::itemDoubleClicked, self, [this](QListWidgetItem* item){
            auto index = ui->playlist->indexFromItem(item).row();
            setCurrentIndex(index);
        });
        QWidget::connect(videoWidget, &VideoWidget::finished, self, [this](){
            auto nexti = nextIndexByOrder(_index);
            setCurrentIndex(nexti);
        });
        QWidget::connect(videoWidget, &VideoWidget::nextVideo, self, [this](){
            auto nexti = nextIndexByOrder(_index);
            if (nexti != -1) {
                setCurrentIndex(nexti);
            }
        });
        QWidget::connect(videoWidget, &VideoWidget::previousVideo, self, [this](){
            auto nexti = previousIndexByOrder(_index);
            if (nexti != -1) {
                setCurrentIndex(nexti);
            }
        });
        QWidget::connect(ui->clearListButton, &QToolButton::clicked, self, [this](bool clicked){
            setCurrentIndex(-1);
            ui->playlist->clear();
            itemModel->clear();
        });
    }

public:

    void setCurrentIndex(int i) {
        if (i >= itemModel->size()) {
            qWarning() << "[PlayerWidget:setCurrentIndex] 请求的下标" << i << "大于当前所有视频的size(" << itemModel->size() << ")";
            return ;
        }
        if (i < -1) {
            qWarning() << "[PlayerWidget:setCurrentIndex] 非法下标" << i << "请输入[" << -1 << "," << "videos.size(" << itemModel->size() << ")]之间的值，-1代表停止播放。";
            i = -1;
        }
        if (-1 != i) {
            auto video = itemModel->item(i);
            videoWidget->playVideo(video);
            ui->playlist->setCurrentRow(i);
            ui->videoTitle->setText(video->title());
        }
        _index = i;
    }

    void setPlayOrder(Order playOrder) {
        _order = playOrder;
    }

    int currentIndex() {
        return _index;
    }

    Order playOrder() {
        return _order;
    }

    int nextIndexByOrder(int index) {
        switch (_order) {
        case Order::IN_ORDER:
            return index + 1 >= itemModel->size() ? -1 : index + 1;
        case Order::LIST_LOOP:
            return (index + 1) % itemModel->size();
        case Order::LIST_RANDOM:
            srand((unsigned int)time(NULL));
            return rand() % itemModel->size();
        case Order::SINGLE_CYCLE:
            return index;
        case Order::STOP:
            return -1;
        }
        return -1;
    }

    int previousIndexByOrder(int index) {
        switch (_order) {
        case Order::IN_ORDER:
            return index - 1 < 0 ? -1 : index - 1;
        case Order::LIST_LOOP:
            return (index - 1 + itemModel->size()) % itemModel->size();
        case Order::LIST_RANDOM:
            srand((unsigned int)time(NULL));
            return rand() % itemModel->size();
        case Order::SINGLE_CYCLE:
            return index;
        case Order::STOP:
            return -1;
        }
        return -1;
    }

public:
    PlayerWidget* self;
    Ui::PlayerView* ui;

    VideoWidget* videoWidget = nullptr;

    PopupWidget* playOrderSettingWidget = nullptr;
    Ui::PlayOrderSettingView* ui_playOrderSettingView = nullptr;

    VideoItemModel* itemModel;

    int _index = -1;
    Order _order = Order::IN_ORDER;
};

PlayerWidget::PlayerWidget(QWidget* parent) : CustomizeTitleWidget(parent), d(new PlayerWidgetPrivate(this)) {
    d->setupUi();
    d->connect();
    d->update();
    setWindowTitle("QZoodPlayer");

    setAcceptDrops(true); // 支持从文件夹拖拽
}

void PlayerWidget::resizeEvent(QResizeEvent *event) {
    static bool is_maximized = false;
	if (isMaximized() && !is_maximized) {
        is_maximized = true;
		d->ui->maximizeButton->setIcon(QIcon(":/icons/minimize_white.png"));
	} else if (is_maximized) {
        is_maximized = false;
		d->ui->maximizeButton->setIcon(QIcon(":/icons/maximize_white.png"));
	}

    CustomizeTitleWidget::resizeEvent(event);
}

void PlayerWidget::dragEnterEvent(QDragEnterEvent *event) {
    event->acceptProposedAction();

    QWidget::dragEnterEvent(event);
}

void PlayerWidget::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();

    // 检查是否包含文件 URL
    if (mimeData->hasUrls()) {
        // 获取第一个文件 URL
        QUrl fileUrl = mimeData->urls()[0];
        
        // 转换为本地文件路径
        QString filePath = fileUrl.toLocalFile();
        d->addVideo(createVideoBLL(filePath));
        d->setCurrentIndex(d->itemModel->size() - 1);
    }

    QWidget::dropEvent(event);
}

bool PlayerWidget::eventFilter(QObject* obj,QEvent* event) {
    if (obj == d->ui->videoPlayContainer){
        if(event->type() == QEvent::Type::Resize) {
            d->videoWidget->resize(d->ui->videoPlayContainer->size());
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PlayerWidget::mouseMoveEvent(QMouseEvent* event) {
    if (movingStatus() && d->ui->titleBar->geometry().contains(event->pos())) {
        if (isMaximized()) {
            showNormal();
            move(event->globalPos() - d->ui->titleBar->rect().bottomRight() / 2);
            diff_pos = d->ui->titleBar->rect().bottomRight() / 2;
        }
        move(event->globalPos() - diff_pos);
        event->accept();
    }
    // 刷新窗体状态
    CustomizeTitleWidget::mouseMoveEvent(event);
}

void PlayerWidget::showEvent(QShowEvent *event) {
    createShadow(d->ui->containerWidget);
}

PlayerWidget::~PlayerWidget() {
    delete d;
}