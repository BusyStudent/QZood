#pragma once

#include "../../BLL/data/videoBLL.hpp"

class VideoWidget;

class VideoWidgetStatus {
public:
    enum ErrorCode : int {
        OK = 0,
        VIDEO_LOADING = 1,
        NO_VIDEO = 2,
        INVALID = 100,
    };
public:
    VideoWidgetStatus(VideoWidget* self);
    virtual QString type() = 0;
    virtual int LoadVideo(const VideoBLLPtr video) = 0;
    virtual int changeSource(const QString &source);
    virtual int changePostion(int pos);
    virtual int play() = 0;
    virtual int pause() = 0;
    virtual int clean();
protected:
    VideoWidget* self;
};

class EmptyStatus : public VideoWidgetStatus {
public:
    EmptyStatus(VideoWidget* self);
    QString type() override;
    int LoadVideo(const VideoBLLPtr video) override;
    int changeSource(const QString &source) override;
    int changePostion(int pos) override;
    int play() override;
    int pause() override;
    int clean() override;
};
class LoadingStatus : public VideoWidgetStatus {
public:
    LoadingStatus(VideoWidget* self);
    QString type() override;
    int LoadVideo(const VideoBLLPtr video) override;
    int changeSource(const QString &source) override;
    int changePostion(int pos) override;
    int play() override;
    int pause() override;
    int clean() override;
};
class ReadyStatus : public VideoWidgetStatus {
public:
    ReadyStatus(VideoWidget* self);
    QString type() override;
    int LoadVideo(const VideoBLLPtr video) override;
    int changePostion(int pos) override;
    int play() override;
    int pause() override;
    int clean() override;
};
class PlayingStatus : public VideoWidgetStatus {
public:
    PlayingStatus(VideoWidget* self);
    QString type() override;
    int LoadVideo(const VideoBLLPtr video) override;
    int play() override;
    int pause() override;
    int clean() override;
};
class PauseStatus : public VideoWidgetStatus {
public:
    PauseStatus(VideoWidget* self);
    QString type() override;
    int LoadVideo(const VideoBLLPtr video) override;
    int play() override;
    int pause() override;
    int clean() override;
};