#pragma once

#include <QSlider>
#include <QLabel>

class CustomSliderPrivate;

class CustomSlider : public QSlider
{
    Q_OBJECT
    public:
        CustomSlider(QWidget *parent = nullptr);
        virtual ~CustomSlider();
        int preloadValue();
        void setShowTipLabel(bool flag);
        bool showTipLabel();
        void setTipLable(QLabel* label);

    Q_SIGNALS:
        void preloadValueChange(int value);
        void tipBeforeShow(QLabel* tipLabel, int value);
        void tipShowed(QLabel* tipLabel, int value);

    public Q_SLOTS:
        void setPreloadValue(int value);

    protected:
        void paintEvent(QPaintEvent *ev) override;
        void mousePressEvent(QMouseEvent *ev) override;
        void mouseMoveEvent(QMouseEvent *ev) override;
        void resizeEvent(QResizeEvent* ev) override;
    
    private:
        CustomSliderPrivate* d;

    friend class CustomSliderPrivate;
};