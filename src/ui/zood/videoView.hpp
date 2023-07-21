#pragma once

#include <QWidget>

#include "../../BLL/data/videoSourceBLL.hpp"

struct VideoData {
    RefPtr<DataObject> videoPtr;
    QString videoTitle;
    QString videoExtraInformation;
    QString videoSourceInformation;
    QImage image;
};

typedef QVector<VideoData> VideoDataVector;

namespace Ui {
class VideoView;
}

class VideoView : public QWidget {
    Q_OBJECT
    public:
        VideoView(QWidget *parent = nullptr);
        virtual ~VideoView();
        RefPtr<DataObject> videoPtr() const;
        QString videoTitle() const;

    public Q_SLOTS:
        void setImage(const QImage &image, const QString &tooltip = QString());
        void setTitle(const QString &str, const QString &tooltip = QString());
        void setExtraInformation(const QString &str, const QString &tooltip = QString());
        void setSourceInformation(const QString &str, const QString &tooltip = QString());
        void setVideoPtr(const RefPtr<DataObject> &videoPtr);
        void setTimelineEpisode(TimelineEpisodePtr tep);
    
    Q_SIGNALS:
        void clicked(const RefPtr<DataObject> &videoPtr);
        void clickedImage(const RefPtr<DataObject> &videoPtr);
        void clickedTitle(const RefPtr<DataObject> &videoPtr, const QString &title);
        void clickedExtraInformation(const RefPtr<DataObject> &videoPtr, const QString &extraInformation);
        void clickedSourceInformation(const RefPtr<DataObject> &videoPtr, const QString &sourceInformation);

    public:
        bool eventFilter(QObject* obj, QEvent *event) override;

    private:
        QScopedPointer<Ui::VideoView> ui;
        RefPtr<DataObject> mVideoPtr = nullptr;
};