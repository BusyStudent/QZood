#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>

class SearchBox : public QLineEdit {
	Q_OBJECT
	public:
		SearchBox(QWidget *parent = nullptr);
		~SearchBox();

	public Q_SLOTS:
		void searchTriggered(bool checked);
		void clearTriggered(bool checked);

	Q_SIGNALS:
		void searchRequested(const QString &text);

	private:
		QAction *clearAction;
		QAction *searchAction;
};