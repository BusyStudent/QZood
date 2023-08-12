#pragma once

#include <QWidget>
#include <QString>

#include "../../util/interface/settingItem.hpp"

class SubtitleSettingPrivate;

class SubtitleSetting : public QWidget, public SettingItem {
Q_OBJECT
public:
    SubtitleSetting(QWidget *parent = nullptr);
    ~SubtitleSetting();
    void initialize(VideoWidget *videoWidget) override;
    void refresh() override;
    void reset() override;
    QString title() override;
    QWidget *widget() override;
private:
    QScopedPointer<SubtitleSettingPrivate> d;
};
