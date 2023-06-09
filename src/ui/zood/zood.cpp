#include "zood.hpp"

#include <QMenu>
#include <QMenuBar>
#include <QPropertyAnimation>
#include <QStringListModel>
#include <QComboBox>
#include <QAbstractItemView>
#include <QScrollBar>

#include "ui_zood.h"
#include "homeWidget.hpp"

Zood::Zood(QWidget *parent) : CustomizeTitleWidget(parent), ui(new Ui::Zood()) {
    // 得到设计器生成的类指针。
    // 设置设计器设计的UI布局。
    ui->setupUi(this);
    setWindowTitle("QZood");
    // 应用自定义窗口基类定义的容器shadow
    createShadow(ui->containerWidget);
    
    homePage = new HomeWidget();
    ui->centerWidget->addWidget(homePage);

    connect(ui->homeButton, &QToolButton::clicked, this, [this](){
        ui->centerWidget->setCurrentWidget(homePage);
    });

    // 连接默认窗口按钮的功能
    connect(ui->minimizeButton, &QToolButton::clicked, this, [this](){
        showMinimized();
    });
    connect(ui->maximizeButton, &QToolButton::clicked, this, [this](){
        if (isMaximized()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(ui->closeButton, &QToolButton::clicked, this, [this](){
        close();
    });

    // 定义搜索框的行为。
    // --定义活动按钮
    searchEdit = new QLineEdit();
    searchCompleter = new QCompleter();

    searchEdit->setClearButtonEnabled(true);//添加清除按钮

    searchEdit->addAction(ui->actionSearch, QLineEdit::ActionPosition::LeadingPosition);

    connect(ui->actionSearch, &QAction::triggered, searchEdit, [this](bool checked) {
        // TODO(llhsdmd@qq.com): 单击搜索按钮，进入搜索结果界面并加载搜索结果。
    });
    connect(searchEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        if (text.isEmpty()) {
        } else {
            ui->searchBox->clear();
            ui->searchBox->addItem(text);
            editTextChanged(text);
        }
    });

    ui->searchBox->installEventFilter(this);
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

void Zood::setPredictStringList(QStringList indicator) {
    auto model = static_cast<QStringListModel *>(searchCompleter->model());

    model->removeRows(0, model->rowCount());
    model->setStringList(indicator);

    ui->searchBox->addItems(indicator);
    auto view = searchCompleter->popup();
    view->move(view->pos() + QPoint(0, 5));
}

void Zood::resizeEvent(QResizeEvent *event) {
    static bool is_maximized = false;
	if (isMaximized() && !is_maximized) {
        is_maximized = true;
		ui->maximizeButton->setIcon(QIcon(":/icons/minimize.png"));
	} else if (is_maximized) {
        is_maximized = false;
		ui->maximizeButton->setIcon(QIcon(":/icons/maximize.png"));
	}

    CustomizeTitleWidget::resizeEvent(event);
}

void Zood::mouseMoveEvent(QMouseEvent* event) {
    if (movingStatus() && ui->topBarWidget->geometry().contains(event->pos())) {
        window()->move(event->globalPos() - diff_pos);
        event->accept();
    }

    CustomizeTitleWidget::mouseMoveEvent(event);
}

bool Zood::eventFilter(QObject *obj, QEvent *e) {
    if (obj == ui->searchBox) {
        if (e->type() == QEvent::FocusIn) {
            QPropertyAnimation *animation = new QPropertyAnimation(ui->searchBox, "geometry");
            animation->setDuration(500); // 设置动画持续时间为0.5秒
            auto rect = ui->searchBox->geometry();
            auto container_rect = ui->searchBoxLayout->geometry();
            animation->setStartValue(rect); // 设置起始值为左对齐
            animation->setEndValue(QRect{(container_rect.width() - rect.width()) / 2, rect.y(), rect.width(), rect.height()}); // 设置结束值为居中对齐
            animation->setEasingCurve(QEasingCurve::InOutQuad); // 设置缓动曲线为InOutQuad

            // 启动动画
            animation->start();
        } else if (e->type() == QEvent::FocusOut) {
            QPropertyAnimation *animation = new QPropertyAnimation(ui->searchBox, "geometry");
            animation->setDuration(500); // 设置动画持续时间为1秒
            auto rect = ui->searchBox->geometry();
            auto container_rect = ui->searchBoxContainer->geometry();
            animation->setStartValue(rect); // 设置起始值为左对齐
            animation->setEndValue(QRect{(container_rect.width() - rect.width()), rect.y(), rect.width(), rect.height()}); // 设置结束值为居中对齐
            animation->setEasingCurve(QEasingCurve::InOutQuad); // 设置缓动曲线为InOutQuad

            // 启动动画
            animation->start();
        }
    }

    return CustomizeTitleWidget::eventFilter(obj, e);
}
