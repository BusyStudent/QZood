#pragma once

#include <QString>
#include <QListWidget>

#include "../../net/datalayer.hpp"

class VideoBLL;

using VideoBLLPtr = RefPtr<VideoBLL>;

class VideoBLL {
    public:
        static VideoBLLPtr createVideoBLL(const QString filepath);
        static VideoBLLPtr createVideoBLL(const EpisodePtr episode);

        virtual QListWidgetItem* addToList(QListWidget* listWidget);
        virtual QString loadVideo();
        virtual QString title();
    
    protected:
        VideoBLL() = default;
        VideoBLL(const EpisodePtr episode);
    
    private:
        EpisodePtr video;
};

// TODO(llhsdmd) : 等接口,先存地址做本地测试
class VideoBLLLocal : VideoBLL {
    public:
        QListWidgetItem* addToList(QListWidget* listWidget) override;
        QString loadVideo() override;
        QString title() override;
    
    protected:
        VideoBLLLocal(const QString filepath);

    private:
        QString filePath;

    friend class VideoBLL;
};