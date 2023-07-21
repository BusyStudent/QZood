/**
 * @file videoSourceBLL.h
 * @brief 数据源业务逻辑层
 *
 * 这个文件包含了数据源单例的逻辑层抽象，提供了底层数据接口到业务逻辑需求的适配。
 */
#pragma once

#include <QList>
#include <QApplication>
#include <QImage>

#include "../../net/promise.hpp"
#include "videoBLL.hpp"

const QImage kLoadingImage = QImage(":/icons/loading_bar.png");

class VideoInterface;
class VideoSourceBLLHelper;

enum TimeWeek : int {
    MONDAY = 1,
    TUESDAY = 2,
    WEDNESDAY = 3,
    THURSDAY = 4,
    FRIDAY = 5,
    SATURDAY = 6,
    SUNDAY = 7,
};

/**
 * @class VideoSouceBLL
 * @brief 视频源逻辑层单例
 *
 * 提供数据给UI层进行UI界面更新
 */
class VideoSourceBLL : public QObject{
    public:
        enum Update : uint8_t {
            TIMELINE = 1u<<0,
        };

    public:
        static inline VideoSourceBLL* instance() {
            static VideoSourceBLL* v = new VideoSourceBLL(qApp);
            return v;
        }
        ~VideoSourceBLL();

        /**
         * @brief 搜索
         *
         * 根据提供的文本对所有视频源进行搜索。
         *
         * @param text 待搜索的文本
         * @param obj 绑定到该对象的生命周期，nullptr时绑定到该类自己身上
         * @param func 得到搜索结果时调用
         */
        void searchSuggestion(const QString& text, QObject *obj, std::function<void(const Result<QStringList>&)> func);
        /**
         * @brief 搜索时间线
         *  
         * 搜索所有源的番剧时间线，
         * 
         * @param obj 绑定到该对象的生命周期，nullptr时绑定到该类自己身上
         * @param func 得到搜索结果时的调用
         */
        void searchVideoTimeline(QObject *obj, std::function<void(const Result<Timeline>&)> func);
        /**
         * @brief 搜索时间线上的所有番剧
         * 
         *  搜索给定时间线集合内所有的番剧，并返回对应的番剧列表。
         * 
         * @param t 番剧时间线
         * @param obj 绑定到该对象的生命周期，nullptr时绑定到该类自己身上
         * @param func 得到搜索结果时的调用
         */
        void searchBangumiFromTimeline(Timeline t,QObject *obj, std::function<void(const Result<TimelineEpisodeList>&)> func);
        /**
         * @brief 搜索给定番剧的所有视频
         * 
         * @param b 番剧
         * @param obj 绑定到该对象的生命周期，nullptr时绑定到该类自己身上
         * @param func 得到搜索结果时的调用
         */
        void searchVideosFromBangumi(BangumiPtr b, QObject *obj, std::function<void(const Result<QList<VideoBLLPtr>>&)> func);
        void searchVideosFromBangumi(RefPtr<DataObject> b, QObject *obj, std::function<void(const Result<QList<VideoBLLPtr>>&)> func);
        /**
         * @brief 通过标题搜索视频
         * 
         * @param title 视频标题
         * @param obj 绑定到该对象的生命周期，nullptr时绑定到该类自己身上
         * @param func 得到搜索结果时的调用
         */
        void searchVideosFromTitle(const QString& title, QObject *obj, std::function<void(const Result<QList<VideoBLLPtr>>&)> func);
        /**
         * @brief 通过文本获取番剧
         * 
         * @param text 文本
         * @param obj 绑定到该对象的生命周期，nullptr时绑定到该类自己身上
         * @param func 得到搜索结果时的调用
         */
        void searchBangumiFromText(const QString& text, QObject *obj, std::function<void(const Result<BangumiList>&)> func);
        /**
         * @brief 筛选时间线中指定时间的番剧
         * 
         * @param week 时间
         * @return Timeline 筛选出来的时间线 
         */
        Timeline videoInWeek(TimeWeek week);

    private:
        VideoSourceBLL(QObject *parent = nullptr);
        VideoSourceBLL(const VideoSourceBLL&) = delete;
        VideoSourceBLL operator=(const VideoSourceBLL&) = delete;
        QString mapToTitle(BangumiPtr b);

    private:
        RefPtr<VideoSourceBLLHelper> d;
        QList<TimelineItemPtr> videoTimelineItems;
        QMap<TimeWeek, TimelineEpisodeList> bangumis;
        QMap<QString, QList<VideoBLLPtr>> videos;
        QMap<QString, TimelineEpisodePtr> titleToTimelineEpisodeList;
        uint8_t dirty = 0xff;
};