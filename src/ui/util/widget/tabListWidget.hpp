#pragma once

#include <QListWidget>

class TabListWidget : public QListWidget {
Q_OBJECT
public:
    TabListWidget(QWidget* parent = nullptr);
    ~TabListWidget();
    QSize sizeHint() const override;
};