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
#include "../../common/myGlobalLog.hpp"
#include "ui_zood.h"

class ZoodPrivate {
public:
    ZoodPrivate(Zood* parent) : self(parent), mUi(new Ui::Zood()) { }

    void setupUi() {
        mUi->setupUi(self);
        mHomePage = new HomeWidget();
        mUi->centerWidget->addWidget(mHomePage);

        setupSearchWidget();
    }

    void setupSearchWidget() {
        mSearchEdit = new QLineEdit();
        mSearchCompleter = new QCompleter();
        mSearchEdit->setClearButtonEnabled(true);//添加清除按钮
        mSearchEdit->addAction(mUi->actionSearch, QLineEdit::ActionPosition::LeadingPosition);

        mUi->searchBox->installEventFilter(self);
        mUi->searchBox->setLineEdit(mSearchEdit);
        mUi->searchBox->setCompleter(mSearchCompleter);

        auto string_list_model = new QStringListModel();
        mSearchCompleter->setModel(string_list_model);
        mSearchCompleter->setCaseSensitivity(Qt::CaseSensitive);
        mSearchCompleter->setFilterMode(Qt::MatchRecursive);
        mSearchCompleter->setCompletionMode(QCompleter::CompletionMode::UnfilteredPopupCompletion);
        auto searchCompleterPopupWidget = mSearchCompleter->popup();
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

    void moveSearchWidgetAnimation(const QRect& startRect, const QRect& endRect)
    {
        QPropertyAnimation* animation(new QPropertyAnimation(mUi->searchBox, "geometry"));
        animation->setDuration(500); // 设置动画持续时间为0.5秒
        animation->setStartValue(startRect); // 设置起始值为左对齐
        animation->setEndValue(endRect); // 设置结束值为居中对齐
        animation->setEasingCurve(QEasingCurve::InOutQuad); // 设置缓动曲线为InOutQuad
        QWidget::connect(animation, &QPropertyAnimation::finished, self, [animation]() {
            animation->deleteLater();
            });
        // 启动动画
        animation->start();
    }

private:
    void connectTitleBar() {
        // 连接默认窗口按钮的功能
        QWidget::connect(mUi->minimizeButton, &QToolButton::clicked, self, [this](){
            self->showMinimized();
        });
        QWidget::connect(mUi->maximizeButton, &QToolButton::clicked, self, [this](){
            if (self->isMaximized()) {
                self->showNormal();
            } else {
                self->showMaximized();
            }
        });
        QWidget::connect(mUi->closeButton, &QToolButton::clicked, self, [this](){
            self->close();
        });
        QWidget::connect(mUi->homeButton, &QToolButton::clicked, self, [this](){
            mUi->centerWidget->setCurrentWidget(mHomePage);
        });
    }
    void connectSearch() {
        QWidget::connect(mUi->actionSearch, &QAction::triggered, mSearchEdit, [this](bool checked) {
            // TODO(llhsdmd@qq.com): 单击搜索按钮，进入搜索结果界面并加载搜索结果。
        });
        QWidget::connect(mSearchEdit, &QLineEdit::textEdited, self, [this](const QString &text) {
            if (text.isEmpty()) {
            } else {
                mUi->searchBox->clear();
                mUi->searchBox->addItem(text);
                self->editTextChanged(text);
            }
        });
    }

public:
    QScopedPointer<Ui::Zood> mUi;

    HomeWidget* mHomePage;
    QLineEdit* mSearchEdit = nullptr;
    QCompleter* mSearchCompleter = nullptr;

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
    return d->mHomePage;
}
QLineEdit* Zood::searchBox() {
    return d->mSearchEdit;
}

void Zood::setPredictStringList(QStringList indicator) {
    auto model = static_cast<QStringListModel *>(d->mSearchCompleter->model());

    model->removeRows(0, model->rowCount());
    model->setStringList(indicator);

    d->mUi->searchBox->addItems(indicator);
    auto view = d->mSearchCompleter->popup();
    view->move(view->pos() + QPoint(0, 5));
}

void Zood::showEvent(QShowEvent *event) {
    createShadow(d->mUi->containerWidget);
}

void Zood::resizeEvent(QResizeEvent *event) {
    static bool is_maximized = false;
	if (isMaximized() && !is_maximized) {
        is_maximized = true;
		d->mUi->maximizeButton->setIcon(QIcon(":/icons/minimize.png"));
	} else if (is_maximized) {
        is_maximized = false;
		d->mUi->maximizeButton->setIcon(QIcon(":/icons/maximize.png"));
	}

    CustomizeTitleWidget::resizeEvent(event);
}

void Zood::mouseMoveEvent(QMouseEvent* event) {
    CustomizeTitleWidget::mouseMoveEvent(event);
}

bool Zood::eventFilter(QObject *obj, QEvent *e) {
    if (obj == d->mUi->searchBox) {
        if (e->type() == QEvent::FocusIn) {
            auto rect = d->mUi->searchBox->geometry();
            auto container_rect = d->mUi->searchBoxContainer->geometry();
            d->moveSearchWidgetAnimation(rect, QRect((container_rect.width() - rect.width()) / 2, rect.y(), rect.width(), rect.height()));
        } else if (e->type() == QEvent::FocusOut) {
            QPropertyAnimation* animation(new QPropertyAnimation(d->mUi->searchBox, "geometry"));
            auto rect = d->mUi->searchBox->geometry();
            auto container_rect = d->mUi->searchBoxContainer->geometry();
            d->moveSearchWidgetAnimation(rect, QRect(container_rect.width() - rect.width(), rect.y(), rect.width(), rect.height()));
        }
    }

    return CustomizeTitleWidget::eventFilter(obj, e);
}

bool Zood::isInTitleBar(const QPoint &pos) {
    auto barLocalPos = d->mUi->topBarWidget->mapFrom(this, pos);
    return d->mUi->topBarWidget->geometry().contains(pos) &&
           !d->mUi->searchBox->geometry().contains(d->mUi->searchBoxContainer->mapFrom(this, pos)) && 
           !d->mUi->minimizeButton->geometry().contains(barLocalPos) &&
           !d->mUi->maximizeButton->geometry().contains(barLocalPos) &&
           !d->mUi->closeButton->geometry().contains(barLocalPos);
}