#pragma once

#include <QWidget>
#include <QString>

#include "../../util/interface/settingItem.hpp"

class ScreenSettingPrivate;

class ScreenSetting : public QWidget, public SettingItem {
Q_OBJECT
public:
    ScreenSetting(QWidget *parent = nullptr);
    ~ScreenSetting();
    void initialize(VideoWidget *videoWidget) override;
    void refresh() override;
    void reset() override;
    QString title() override;
    QWidget *widget() override;
private:
    QScopedPointer<ScreenSettingPrivate> d;
};
