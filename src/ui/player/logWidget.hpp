#include "../common/popupWidget.hpp"

class LogWidgetPrivate;

class LogWidget : public PopupWidget {
    Q_OBJECT
    public:
        LogWidget(QWidget *parent = nullptr);
        ~LogWidget();
        void pushLog(const QString& log);

    private:
        LogWidgetPrivate *d = nullptr;

    friend class LogWidgetPrivate;
};