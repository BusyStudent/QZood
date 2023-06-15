#include "videoItemModel.hpp"

VideoItemModel::VideoItemModel() {}
int VideoItemModel::index(VideoBLLPtr video) {
    for (int i = 0;i < videos.size(); ++i) {
        if (video == videos[i]) {
            return i;
        }
    }
    return -1;
}

void VideoItemModel::add(VideoBLLPtr video) {
    videos.push_back(video);
    flags.push_back(ItemFlag::DIRTY);
}

void VideoItemModel::insert(int index, VideoBLLPtr item) {
    videos.insert(index, item);
    flags.insert(index, ItemFlag::DIRTY);
}

void VideoItemModel::remove(int index) {
    videos.remove(index);
    flags.remove(index);
}

void VideoItemModel::clear() {
    videos.clear();
    flags.clear();
}

VideoBLLPtr VideoItemModel::item(int idx) {
    return videos[idx];
}

void VideoItemModel::setFlag(int index, ItemFlag flag, bool v) {
    flags[index] = flags[index] | flag;
    if (!v) {
        flags[index] = flags[index] ^ flag;
    }
}

ItemFlag VideoItemModel::flag(int index) {
    return flags[index];
}

int VideoItemModel::size() {
    return videos.size();
}
