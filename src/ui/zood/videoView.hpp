#pragma once

#include <QWidget>

struct videoData {
    QString videoId;
    QString videoTitle;
    QString videoExtraInformation;
    QString videoSourceInformation;
    QImage image;
};

namespace Ui {
class VideoView;
}

class VideoView : public QWidget {
    Q_OBJECT
    public:
        VideoView(QWidget *parent = nullptr);
        virtual ~VideoView();

    public Q_SLOTS:
        void setImage(const QImage &image, const QString &tooltip = QString());
        void setTitle(const QString &str, const QString &tooltip = QString());
        void setExtraInformation(const QString &str, const QString &tooltip = QString());
        void setSourceInformation(const QString &str, const QString &tooltip = QString());
        void setVideoId(const QString &videoId);
    
    Q_SIGNALS:
        void clickedImage(const QString &videoId);
        void clickedTitle(const QString &videoId, const QString &title);
        void clickedExtraInformation(const QString &videoId, const QString &extraInformation);
        void clickedSourceInformation(const QString &videoId, const QString &sourceInformation);

    public:
        bool eventFilter(QObject* obj, QEvent *event) override;

    private:
        QScopedPointer<Ui::VideoView> ui;
        QString videoId = "";
};