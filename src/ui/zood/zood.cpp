#include "zood.hpp"

#include <QMenu>
#include <QMenuBar>
#include <QPropertyAnimation>
#include <QStringListModel>
#include <QComboBox>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>

#include "homeWidget.hpp"
#include "localWidget.hpp"
#include "homeWidget.hpp"
#include "../../net/client.hpp"
#include "../../BLL/data/videoSourceBLL.hpp"
#include "ui_zood.h"

class ZoodPrivate {
public:
    ZoodPrivate(Zood* parent) : self(parent), ui(new Ui::Zood()) { }

    void setupUi() {
        ui->setupUi(self);
        homePage = new HomeWidget();
        ui->centerWidget->addWidget(homePage);

        // 定义搜索框的行为。
        // --定义活动按钮
        searchEdit = new QLineEdit();
        searchCompleter = new QCompleter();
        searchEdit->setClearButtonEnabled(true);//添加清除按钮
        searchEdit->addAction(ui->actionSearch, QLineEdit::ActionPosition::LeadingPosition);

        ui->searchBox->installEventFilter(self);
        ui->searchBox->setLineEdit(searchEdit);
        ui->searchBox->setCompleter(searchCompleter);
        
        auto string_list_model = new QStringListModel();
        searchCompleter->setModel(string_list_model);
        searchCompleter->setCaseSensitivity(Qt::CaseSensitive);
        searchCompleter->setFilterMode(Qt::MatchRecursive);
        searchCompleter->setCompletionMode(QCompleter::CompletionMode::UnfilteredPopupCompletion);
        auto searchCompleterPopupWidget = searchCompleter->popup();
        searchCompleterPopupWidget->setStyleSheet(
            R"(QAbstractItemView{
                border:1px solid #F5F5F5;
                border-radius: 15px;
                padding: 1px;
                outline: 0px;}
            QAbstractItemView::item{
                border: 1px solid white;
                border-radius: 11px;
                padding-top: 2px;
                padding-bottom: 2px;}
            QAbstractItemView::item:hover{
                background-color:lightGrey;}
            QAbstractItemView::item:selected{
                background-color:lightGrey;
                border:1px solid lightGrey;
                show-decoration-selected: 0;
                border-radius: 11px;
                color: black;})"
            );
        searchCompleterPopupWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        searchCompleterPopupWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    void connect() {
        connectTitleBar();
        connectSearch();
    }

private:
    void connectTitleBar() {
        // 连接默认窗口按钮的功能
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
            self->close();
        });
        QWidget::connect(ui->homeButton, &QToolButton::clicked, self, [this](){
            ui->centerWidget->setCurrentWidget(homePage);
        });
    }
    void connectSearch() {
        QWidget::connect(ui->actionSearch, &QAction::triggered, searchEdit, [this](bool checked) {
            // TODO(llhsdmd@qq.com): 单击搜索按钮，进入搜索结果界面并加载搜索结果。
        });
        QWidget::connect(searchEdit, &QLineEdit::textEdited, self, [this](const QString &text) {
            if (text.isEmpty()) {
            } else {
                ui->searchBox->clear();
                ui->searchBox->addItem(text);
                self->editTextChanged(text);
            }
        });
    }

public:
    QScopedPointer<Ui::Zood> ui;

    HomeWidget* homePage;
    QLineEdit* searchEdit = nullptr;
    QCompleter* searchCompleter = nullptr;

    QList<VideoInterface*> videoInterface;

private:
    Zood *self = nullptr;
};

Zood::Zood(QWidget *parent) : CustomizeTitleWidget(parent), d(new ZoodPrivate(this)) {
    setWindowTitle("QZood");

    d->setupUi();
    d->connect();
}

Zood::~Zood() { }

HomeWidget* Zood::homeWidget() {
    return d->homePage;
}
QLineEdit* Zood::searchBox() {
    return d->searchEdit;
}

void Zood::setPredictStringList(QStringList indicator) {
    auto model = static_cast<QStringListModel *>(d->searchCompleter->model());

    model->removeRows(0, model->rowCount());
    model->setStringList(indicator);

    d->ui->searchBox->addItems(indicator);
    auto view = d->searchCompleter->popup();
    view->move(view->pos() + QPoint(0, 5));
}

void Zood::showEvent(QShowEvent *event) {
    createShadow(d->ui->containerWidget);
}

void Zood::resizeEvent(QResizeEvent *event) {
    static bool is_maximized = false;
	if (isMaximized() && !is_maximized) {
        is_maximized = true;
		d->ui->maximizeButton->setIcon(QIcon(":/icons/minimize.png"));
	} else if (is_maximized) {
        is_maximized = false;
		d->ui->maximizeButton->setIcon(QIcon(":/icons/maximize.png"));
	}

    CustomizeTitleWidget::resizeEvent(event);
}

void Zood::mouseMoveEvent(QMouseEvent* event) {
    if (movingStatus() && d->ui->topBarWidget->geometry().contains(event->pos())) {
        window()->move(event->globalPos() - diff_pos);
        event->accept();
    }

    CustomizeTitleWidget::mouseMoveEvent(event);
}

bool Zood::eventFilter(QObject *obj, QEvent *e) {
    if (obj == d->ui->searchBox) {
        if (e->type() == QEvent::FocusIn) {
            QPropertyAnimation* animation(new QPropertyAnimation(d->ui->searchBox, "geometry"));
            animation->setDuration(500); // 设置动画持续时间为0.5秒
            auto rect = d->ui->searchBox->geometry();
            auto container_rect = d->ui->searchBoxLayout->geometry();
            animation->setStartValue(rect); // 设置起始值为左对齐
            animation->setEndValue(QRect{(container_rect.width() - rect.width()) / 2, rect.y(), rect.width(), rect.height()}); // 设置结束值为居中对齐
            animation->setEasingCurve(QEasingCurve::InOutQuad); // 设置缓动曲线为InOutQuad
            connect(animation, &QPropertyAnimation::finished, this, [animation](){
                animation->deleteLater();
            });
            // 启动动画
            animation->start();
        } else if (e->type() == QEvent::FocusOut) {
            QPropertyAnimation* animation(new QPropertyAnimation(d->ui->searchBox, "geometry"));
            animation->setDuration(500); // 设置动画持续时间为1秒
            auto rect = d->ui->searchBox->geometry();
            auto container_rect = d->ui->searchBoxContainer->geometry();
            animation->setStartValue(rect); // 设置起始值为左对齐
            animation->setEndValue(QRect{(container_rect.width() - rect.width()), rect.y(), rect.width(), rect.height()}); // 设置结束值为居中对齐
            animation->setEasingCurve(QEasingCurve::InOutQuad); // 设置缓动曲线为InOutQuad
            connect(animation, &QPropertyAnimation::finished, this, [animation](){
                animation->deleteLater();
            });
            // 启动动画
            animation->start();
        }
    }

    return CustomizeTitleWidget::eventFilter(obj, e);
}
