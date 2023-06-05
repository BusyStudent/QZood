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
        void setParent(QWidget *parent);
        void setParent(QWidget *parent, Qt::WindowFlags f);
        void enterEvent(QEnterEvent* event) override;
        void leaveEvent(QEvent* event) override;
        bool eventFilter(QObject* obj, QEvent* event) override;

    public Q_SLOTS:
        void show();
        void hideLater(int msec = -1); 
        void stopHideTimer();
        void hide();

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