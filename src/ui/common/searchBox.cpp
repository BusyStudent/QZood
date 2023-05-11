#include "searchBox.hpp"

#include <QAction>
#include <QResource>

SearchBox::SearchBox(QWidget *parent) : QLineEdit(parent) {
  clearAction = new QAction();
  searchAction = new QAction();
  setObjectName("searchBox");

  clearAction->setIcon(QIcon(":/icons/cross.png"));
  searchAction->setIcon(QIcon(":/icons/search.png"));

  addAction(searchAction, QLineEdit::ActionPosition::LeadingPosition);
  addAction(clearAction, QLineEdit::ActionPosition::TrailingPosition);

  connect(clearAction, &QAction::triggered, this, &SearchBox::clearTriggered);
  connect(searchAction, &QAction::triggered, this, &SearchBox::searchTriggered);
  connect(this, &SearchBox::editingFinished, this,
          [this]() { searchRequested(text()); });
  connect(this, &SearchBox::textChanged, this, [this](const QString &text) {
    if (text.isEmpty()) {
      clearAction->setVisible(false);
    } else {
      clearAction->setVisible(true);
    }
  });

  clearAction->setVisible(false);

  setStyleSheet(
      "QLineEdit#searchBox{border: 2px solid black}"
      "QLineEdit#searchBox{border-radius: 10px}"
      "QLineEdit#searchBox::focus{border-color: #ff0000}");
}

SearchBox::~SearchBox() {}

void SearchBox::searchTriggered(bool checked) { searchRequested(text()); }
void SearchBox::clearTriggered(bool checked) { clear(); }