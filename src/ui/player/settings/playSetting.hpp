#pragma once

#include <QWidget>
#include <QString>

#include "../../util/interface/settingItem.hpp"

class PlaySettingPrivate;

class PlaySetting : public QWidget, public SettingItem {
Q_OBJECT
public:
    PlaySetting(QWidget *parent = nullptr);
    ~PlaySetting();
    void initialize(VideoWidget *videoWidget) override;
    void refresh() override;
    void reset() override;
    QString title() override;
    QWidget *widget() override;
private:
    QScopedPointer<PlaySettingPrivate> d;
};
