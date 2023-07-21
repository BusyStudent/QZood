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
            mAligns = align;
        }
        inline void setAssociateWidget(QWidget* widget) {
            mAttachWidget = widget;
        }
        inline void setAuotLayout(bool flag = true) { mAutoLayout = flag; }
        inline bool autoLayout() { return mAutoLayout; }
        inline void setHideAfterLeave(bool flag = true) { mHideAfterLeave = flag; }
        inline bool hideAfterLeave() { return mHideAfterLeave; }
        inline void setStopTimerEnter(bool flag = true) { mStopTimerEnter = flag; }
        inline bool stopTimerEnter() { return mStopTimerEnter; }
        inline void setDefualtHideTime(int ms) { mDefualtHideAfterTime = ms; }
        inline bool defualtHideTime() { return mDefualtHideAfterTime; }
        inline void setOutside(bool flag = true) { mOutside = flag; }
        inline bool outside() { return mOutside; }

    public:
        void enterEvent(QEnterEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
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
        QTimer* mTimer = nullptr;
        QWidget* mAttachWidget = nullptr;
        Qt::Alignment mAligns = Qt::AlignBottom | Qt::AlignHCenter;
        bool mAutoLayout = false;
        bool mHideAfterLeave = true;
        bool mStopTimerEnter = true;
        int mDefualtHideAfterTime = 100;
        bool mOutside = true;
};