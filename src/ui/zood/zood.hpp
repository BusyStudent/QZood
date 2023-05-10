#pragma once

#include "../common/searchBox.hpp"
#include "../common/customizeTitleWidget.hpp"
#include "homeWidget.hpp"
#include "localWidget.hpp"

#include <QResizeEvent>
#include <QStackedWidget>

class Zood : public CustomizeTitleWidget {
	Q_OBJECT
	public:
		Zood(QWidget *parent = nullptr);
	
	public:
		void resizeEvent(QResizeEvent *event) override;

	private:
		QLabel *title;

		QToolButton *home;
		QToolButton *local;

		SearchBox *searchEdit;

		QMenuBar *mainMenu;
		QMenu *history;
		QMenu *setting;
        QAction *addSource;
        QAction *editSource;
        QAction *languageSetting;
        QAction *theme;
        QAction *about;

		QAction *minimumAction;
		QAction *maximumAction;
		QAction *closeAction;

        QStackedWidget *centerWidget;
        HomeWidget* homeWidget;
        LocalWidget* localWidget;
};