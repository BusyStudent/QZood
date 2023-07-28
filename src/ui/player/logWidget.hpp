#include "../util/widget/popupWidget.hpp"
#include "ui_customLabel.h"

class LogWidgetPrivate;

class LogWidget : public PopupWidget {
    Q_OBJECT
    public:
        LogWidget(QWidget *parent = nullptr);
        ~LogWidget();
        void pushLog(const QString& log);

    private:
        QScopedPointer<LogWidgetPrivate> d;

    friend class LogWidgetPrivate;
};