#pragma once

#include <QWidget>

struct videoData {
    int videoId;
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
        VideoView(QWidget* parent = nullptr);
        virtual ~VideoView() {}

    public Q_SLOTS:
        void setImage(const QImage& image, const QString& tooltip = QString());
        void setTitle(const QString& str, const QString& tooltip = QString());
        void setExtraInformation(const QString& str, const QString& tooltip = QString());
        void setSourceInformation(const QString& str, const QString& tooltip = QString());
        void setVideoId(const int videoId);
    
    Q_SIGNALS:
        void clickedImage(const int videoId);
        void clickedTitle(const int videoId, const QString& title);
        void clickedExtraInformation(const int videoId, const QString& extraInformation);
        void clickedSourceInformation(const int videoId, const QString& sourceInformation);

    public:
        bool eventFilter(QObject* obj, QEvent *event) override;

    private:
        Ui::VideoView *ui;
        int videoId = -1;
};