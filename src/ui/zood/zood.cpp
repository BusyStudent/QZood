#include "zood.hpp"

#include <QMenu>
#include <QMenuBar>

Zood::Zood(QWidget *parent) : CustomizeTitleWidget(parent) {
	title = new QLabel();
	title->setText("Zood");

	home = new QAction();
	home->setIcon(QIcon(":/icons/home.png"));
	local = new QAction();
	local->setIcon(QIcon(":/icons/localFile.png"));

	searchEdit = new SearchBox();

	history = new QAction();
	history->setIcon(QIcon(":/icons/history.png"));
	mainMenu = new QMenuBar();
	setting = new QMenu();
	mainMenu->addMenu(setting);
	setting->setIcon(QIcon(":/icons/settings.png"));
	auto line = new QFrame();
	line->setFrameShape(QFrame::VLine);

	minimumAction = new QAction();
	minimumAction->setIcon(QIcon(":/icons/minus.png"));
	connect(minimumAction, &QAction::triggered, this, [this](){
		showMinimized();
	});
	maximumAction = new QAction();
	maximumAction->setIcon(QIcon(":/icons/maximize.png"));
	connect(maximumAction, &QAction::triggered, this, [this](){
		if (isMaximized()) {
			maximumAction->setIcon(QIcon(":/icons/maximize.png"));
			showNormal();
		} else {
			maximumAction->setIcon(QIcon(":/icons/minimize.png"));
			showMaximized();
		}
	});
	closeAction = new QAction();
	closeAction->setIcon(QIcon(":/icons/close.png"));
	connect(closeAction, &QAction::triggered, this, [this]() {
		close();
	});

	int index = 0;
	title_bar->insertWidget(title, index++);
	insertActionToTitleBar(home, index++);
	insertActionToTitleBar(local, index++);
	title_bar->insertStretch(index++);
	title_bar->insertWidget(searchEdit, index++);
	insertActionToTitleBar(history, index++);
	title_bar->insertWidget(mainMenu, index++);
	title_bar->insertWidget(line, index++);
	insertActionToTitleBar(minimumAction, index++);
	insertActionToTitleBar(maximumAction, index++);
	insertActionToTitleBar(closeAction, index++);
}