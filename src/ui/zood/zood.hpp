#pragma once

#include "../common/customizeTitleWidget.hpp"
#include "homeWidget.hpp"
#include "localWidget.hpp"

#include <QResizeEvent>
#include <QStackedWidget>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>

extern HomeWidget;
extern VideoView;

class Zood : public CustomizeTitleWidget {
	Q_OBJECT
	public:
		Zood(QWidget *parent = nullptr);
	
	public:
		void resizeEvent(QResizeEvent *event) override;
        bool eventFilter(QObject *obj, QEvent *e) override;
        void setPredictStringList(QStringList indicator);

    Q_SIGNALS:
        void editTextChanged(const QString &);

	private:
		void* ui = nullptr;
        HomeWidget *homePage;
        QLineEdit* searchEdit = nullptr;
        QCompleter* searchCompleter = nullptr;
};