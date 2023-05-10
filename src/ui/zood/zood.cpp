#include "zood.hpp"

#include <QMenu>
#include <QMenuBar>

Zood::Zood(QWidget *parent) : CustomizeTitleWidget(parent) {
	title = new QLabel();
	title->setText("Zood");
    title->setFont(QFont("微软雅黑", 12, 5));

    centerWidget = new QStackedWidget();
    setCentralWidget(centerWidget);
    homeWidget = new HomeWidget();
    localWidget = new LocalWidget();
    centerWidget->addWidget(homeWidget);
    centerWidget->addWidget(localWidget);

	home = new QToolButton();
	home->setIcon(QIcon(":/icons/home.png"));
    auto icon_size = QSize(24, 24);
    home->setIconSize(icon_size * 1.2);
    connect(home, &QToolButton::clicked, this, [this, icon_size](bool checked){
        local->setIconSize(icon_size);
        home->setIconSize(icon_size * 1.2);
        centerWidget->setCurrentWidget(homeWidget);
    });
	local = new QToolButton();
	local->setIcon(QIcon(":/icons/localFile.png"));
    local->setIconSize(icon_size);
    connect(local, &QToolButton::clicked, this, [this, icon_size](bool checked){
        home->setIconSize(icon_size);
        local->setIconSize(icon_size * 1.2);
        centerWidget->setCurrentWidget(localWidget);
    });

	searchEdit = new SearchBox();

	mainMenu = new QMenuBar();
    mainMenu->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

	history = new QMenu();
	history->setIcon(QIcon(":/icons/history.png"));
	mainMenu->addMenu(history);

	setting = new QMenu();
	setting->setIcon(QIcon(":/icons/settings.png"));
	mainMenu->addMenu(setting);

    addSource = new QAction();
    addSource->setText(tr("添加源"));
    setting->addAction(addSource);

    editSource = new QAction();
    editSource->setText(tr("编辑源"));
    setting->addAction(editSource);

    setting->addSeparator();

    languageSetting = new QAction();
    languageSetting->setText(tr("语言"));
    setting->addAction(languageSetting);

    theme = new QAction();
    theme->setText(tr("主题"));
    setting->addAction(theme);

    about = new QAction();
    about->setText(tr("关于"));
    setting->addAction(about);

	auto line = new QFrame();
	line->setFrameShape(QFrame::VLine);

	minimumAction = new QAction();
	minimumAction->setIcon(QIcon(":/icons/minus.png"));
	connect(minimumAction, &QAction::triggered, this,
			[this]() { showMinimized(); });
	maximumAction = new QAction();
	maximumAction->setIcon(QIcon(":/icons/maximize.png"));
	connect(maximumAction, &QAction::triggered, this, [this]() {
	if (isMaximized()) {
		showNormal();
	} else {
		showMaximized();
	}
	});
	closeAction = new QAction();
	closeAction->setIcon(QIcon(":/icons/close.png"));
	connect(closeAction, &QAction::triggered, this, [this]() { close(); });

	int index = 0;
	title_bar->insertWidget(title, index++);
	title_bar->insertWidget(home, index++);
	title_bar->insertWidget(local, index++);
	title_bar->insertStretch(index++);
	title_bar->insertWidget(searchEdit, index++);
	title_bar->insertStretch(index++);
	title_bar->insertWidget(mainMenu, index++, 0, Qt::AlignVCenter);
	title_bar->insertWidget(line, index++);
	insertActionToTitleBar(minimumAction, index++);
	insertActionToTitleBar(maximumAction, index++);
	insertActionToTitleBar(closeAction, index++);
}

void Zood::resizeEvent(QResizeEvent *event) {
    static bool is_maximized = false;
	if (isMaximized() && !is_maximized) {
        is_maximized = true;
		maximumAction->setIcon(QIcon(":/icons/minimize.png"));
	} else if (is_maximized) {
        is_maximized = false;
		maximumAction->setIcon(QIcon(":/icons/maximize.png"));
	}
}