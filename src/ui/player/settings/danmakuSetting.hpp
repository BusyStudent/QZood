#pragma once

#include <QWidget>
#include <QString>

#include "../../util/interface/settingItem.hpp"

class DanmakuSettingPrivate;

class DanmakuSetting : public QWidget, public SettingItem {
Q_OBJECT
public:
    DanmakuSetting(QWidget *parent = nullptr);
    ~DanmakuSetting();
    void initialize(VideoWidget *videoWidget) override;
    void refresh() override;
    void reset() override;
    QString title() override;
    QWidget *widget() override;
private:
    QScopedPointer<DanmakuSettingPrivate> d;
};

