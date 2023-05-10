#include "../searchBox.hpp"
#include "../../../tests/testwindow.hpp"

#include <QVBoxLayout>
#include <QMessageBox>

ZOOD_TEST(searchBoxTest) {
	auto widget = new QWidget();
	auto vBoxLayout = new QVBoxLayout();

	widget->setLayout(vBoxLayout);
	
	auto searchBox = new SearchBox();

	vBoxLayout->addWidget(searchBox);

	QWidget::connect(searchBox, &SearchBox::searchRequested, [=](const QString& text){
		ZoodLogString("searchRequested: " + text);
	});
	QWidget::connect(searchBox, &SearchBox::textChanged, [=](const QString& text){
		ZoodLogString("textChanged: " + text);
	});
	QWidget::connect(searchBox, &SearchBox::editingFinished, [=](){
		ZoodLogString("textEditingFinished: " + searchBox->text());
	});

	return widget;
}