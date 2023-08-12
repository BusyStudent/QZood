#pragma once

#include <QString>

class VideoWidget;
class QWidget;

class SettingItem {
public:
    SettingItem() = default;
    virtual void initialize(VideoWidget *videoWidget) = 0;
    virtual void refresh() = 0;
    virtual void reset() = 0;
    virtual QString title() = 0;
    virtual QWidget *widget() = 0;
};