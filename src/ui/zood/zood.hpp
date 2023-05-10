#pragma once

#include "../common/searchBox.hpp"
#include "../common/customizeTitleWidget.hpp"

class Zood : public CustomizeTitleWidget{
	Q_OBJECT
	public:
		Zood(QWidget *parent = nullptr);
	private:
		QLabel *title;

		QAction *home;
		QAction *local;

		SearchBox *searchEdit;

		QAction *history;
		QMenuBar *mainMenu;
		QMenu *setting;

		QAction *minimumAction;
		QAction *maximumAction;
		QAction *closeAction;
};