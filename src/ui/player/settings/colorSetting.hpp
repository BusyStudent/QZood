#pragma once

#include <QWidget>
#include <QString>

#include "../../util/interface/settingItem.hpp"

class ColorSettingPrivate;

class ColorSetting : public QWidget, public SettingItem {
Q_OBJECT
public:
    ColorSetting(QWidget *parent = nullptr);
    ~ColorSetting();
    void initialize(VideoWidget *videoWidget) override;
    void refresh() override;
    void reset() override;
    QString title() override;
    QWidget *widget() override;
private:
    QScopedPointer<ColorSettingPrivate> d;
};

