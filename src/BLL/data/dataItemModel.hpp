#pragma once

enum class ItemFlag : int {
    VALID = 1<<0,
    DIRTY = 1<<1,
    INVALID = 1<<2,
};

inline ItemFlag operator|(ItemFlag lhs, ItemFlag rhs) {
    return static_cast<ItemFlag>(static_cast<int>(lhs) | static_cast<int>(rhs));
}

inline ItemFlag operator&(ItemFlag lhs, ItemFlag rhs) {
    return static_cast<ItemFlag>(static_cast<int>(lhs) & static_cast<int>(rhs));
}

inline ItemFlag operator^(ItemFlag lhs, ItemFlag rhs) {
    return static_cast<ItemFlag>(static_cast<int>(lhs) ^ static_cast<int>(rhs));
}

inline ItemFlag operator~(ItemFlag e) {
    return static_cast<ItemFlag>(~static_cast<int>(e));
}

template<typename ItemType>
class ItemModel {
    public:
        virtual int index(ItemType item) = 0;
        virtual void add(ItemType item) = 0;
        virtual void insert(int index, ItemType item) = 0;
        virtual void remove(int index) = 0;
        virtual void clear() = 0;
        virtual ItemType item(int index) = 0;
        virtual void setFlag(int index, ItemFlag flag, bool v) = 0;
        virtual ItemFlag flag(int index) = 0;
        virtual int size() = 0;
};