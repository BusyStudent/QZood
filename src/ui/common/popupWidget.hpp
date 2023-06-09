#pragma once

#include <QWidget>
#include <QTimer>

class PopupWidget : public QWidget {
    Q_OBJECT
    public:
        enum Direction{
            TOP,
            RIGHT,
            BOTTOM,
            LEFT
        };
    public:
        PopupWidget(QWidget* parent = nullptr,Qt::WindowFlags f = Qt::WindowFlags());
        inline void setAssociateWidget(QWidget* widget, Direction direction) {
            attach_widget = widget;
            this->direction = direction;
            setAuotLayout();
        }
        inline void setAuotLayout(bool flag = true) { auto_layout = flag; }
        inline bool autoLayout() { return auto_layout; }
        inline void setHideAfterLeave(bool flag = true) { hide_after_leave = flag; }
        inline bool hideAfterLeave() { return hide_after_leave; }
        inline void setDefualtHideTime(int ms) { defualt_hide_after_time = ms; }
        inline bool defualtHideTime() { return defualt_hide_after_time; }

    public:
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        bool eventFilter(QObject* obj, QEvent* event) override;
        void hideEvent(QHideEvent *event) override;
        void showEvent(QShowEvent *event) override;

    public Q_SLOTS:
        void hideLater(int msec = -1); 
        void stopHideTimer();

    Q_SIGNALS:
        void showed();
        void hided();

    private:
        QTimer* timer = nullptr;
        QWidget* attach_widget = nullptr;
        Direction direction = BOTTOM;
        bool auto_layout = false;
        bool hide_after_leave = true;
        int defualt_hide_after_time = 100;
};