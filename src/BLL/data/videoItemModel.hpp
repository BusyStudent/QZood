#pragma once
/**
 * @file videoItemModel.hpp
 * 该头文件描述的是视频播放器的视频列表的管理类
 * @author llhsdmd
 * @date 2023-06-13
 */
#include "videoBLL.hpp"
#include "dataItemModel.hpp"
/**
 * @brief 视频数据项的模型
 * 
 * 该类用于存储一个待播放的视频列表，
 */
class VideoItemModel : public ItemModel<VideoBLLPtr> {
    public:
        VideoItemModel();
        int index(VideoBLLPtr video) override;
        void add(VideoBLLPtr video) override;
        void insert(int index, VideoBLLPtr item) override;
        void remove(int index) override;
        void clear() override;
        VideoBLLPtr item(int idx) override;
        void setFlag(int index, ItemFlag flag, bool v = true) override;
        ItemFlag flag(int index) override;
        int size() override;

    private:
        QList<VideoBLLPtr> videos;
        QList<ItemFlag> flags;

};