#pragma once

#include <QWidget>
#include <QTimer>

class PopupWidgetPrivate;

class PopupWidget : public QWidget {
    Q_OBJECT
    public:
        PopupWidget(QWidget* parent = nullptr,Qt::WindowFlags f = Qt::Widget);
        ~PopupWidget();
        inline void setAlignment(Qt::Alignment align) {
            m_aligns = align;
        }
        inline void setAssociateWidget(QWidget* widget) {
            m_attach_widget = widget;
        }
        inline void setAuotLayout(bool flag = true) { m_auto_layout = flag; }
        inline bool autoLayout() { return m_auto_layout; }
        inline void setHideAfterLeave(bool flag = true) { m_hide_after_leave = flag; }
        inline bool hideAfterLeave() { return m_hide_after_leave; }
        inline void setStopTimerEnter(bool flag = true) { m_stop_timer_enter = flag; }
        inline bool stopTimerEnter() { return m_stop_timer_enter; }
        inline void setDefualtHideTime(int ms) { m_defualt_hide_after_time = ms; }
        inline bool defualtHideTime() { return m_defualt_hide_after_time; }
        inline void setOutside(bool flag = true) { m_outside = flag; }
        inline bool outside() { return m_outside; }

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

    protected:
        QScopedPointer<PopupWidgetPrivate> d;
        QTimer* m_timer = nullptr;
        QWidget* m_attach_widget = nullptr;
        Qt::Alignment m_aligns = Qt::AlignBottom | Qt::AlignHCenter;
        bool m_auto_layout = false;
        bool m_hide_after_leave = true;
        bool m_stop_timer_enter = true;
        int m_defualt_hide_after_time = 100;
        bool m_outside = true;
};