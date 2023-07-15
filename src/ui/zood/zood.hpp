#pragma once

#include "../common/customizeTitleWidget.hpp"

class ZoodPrivate;
class HomeWidget;
class QLineEdit;

class Zood : public CustomizeTitleWidget {
    Q_OBJECT
    public:
        Zood(QWidget *parent = nullptr);
        virtual ~Zood();
        HomeWidget* homeWidget();
        QLineEdit* searchBox();

    public:
        void resizeEvent(QResizeEvent *event) override;
        bool eventFilter(QObject *obj, QEvent *e) override;
        void setPredictStringList(QStringList indicator);
        void mouseMoveEvent(QMouseEvent* event) override;
        void showEvent(QShowEvent *event) override;

    private:
        void moveSearchWidgetAnimation(const QRect& startRect, const QRect& endRect);

    Q_SIGNALS:
        void editTextChanged(const QString &);

    private:
        QScopedPointer<ZoodPrivate> d;
};