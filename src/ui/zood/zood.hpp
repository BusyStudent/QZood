#pragma once

#include "../common/customizeTitleWidget.hpp"
#include "homeWidget.hpp"
#include "localWidget.hpp"

#include <QResizeEvent>
#include <QStackedWidget>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>

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
        QLineEdit* searchEdit = nullptr;
        QCompleter* searchCompleter = nullptr;
};