#pragma once

#include "../common/popupWidget.hpp"

namespace Ui {
class FullSettingView;
}

class FullSettingWidget : public PopupWidget {
    Q_OBJECT
    public:
        FullSettingWidget(QWidget* parent = nullptr);

    public Q_SLOTS:
        void show();
        void setPlaybackRate(double value);
        void setSkipStep(int value);

    Q_SIGNALS:
        void playbackRateChanged(int value);

    private:
        void _setupUi();
        void _setup();
        inline double _playbackRate(int value) { return (double)value / 10; }
        inline int _playbackRate(double value) { return int(value * 10); }

    private:
        Ui::FullSettingView* ui;
};