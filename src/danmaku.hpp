#pragma once

#include <QString>
#include <QList>
#include <QColor>
#include "stl.hpp"

class DanmakuItem {
    public:
        // 类型
        enum Type : int {
            Regular1 = 1,
            Regular2 = 2,
            Regular3 = 3,
            Bottom = 4,
            Top = 5,
            Reserve = 6,
            Advanced = 7,
            Code = 8,
            Bas = 9,
        } type;

        // 弹幕池
        enum Pool : int {
            RegularPool = 1,
            SubtitlePool = 2,
            SpecialPool = 3,
        } pool;
        

        // 大小
        enum Size : int {
            Small = 18,
            Medium = 25,
            Large = 32,
        } size;

        QColor color; //< 颜色
        QString text;

        double position;

        // 屏蔽等级
        uint32_t level;

        bool is_regular() const noexcept {
            return type == Regular1 || type == Regular2 || type == Regular3;
        }
        bool is_bottom() const noexcept {
            return type == Bottom;
        }
        bool is_top() const noexcept {
            return type == Top;
        }
};


using DanmakuList = QList<DanmakuItem>;

/**
 * @brief Parse the danmaku
 * 
 * @param xmlstr 
 * @return Result<DanmakuList> 
 */
Result<DanmakuList> ParseDanmaku(const QString &xmlstr);