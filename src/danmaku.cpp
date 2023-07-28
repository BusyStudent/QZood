#include <libxml/xmlreader.h>
#include <algorithm>
#include "danmaku.hpp"
#include "common/myGlobalLog.hpp"

Result<DanmakuList> ParseDanmaku(const QString &xmlstr) {
    // Parse xml
    auto u8 = xmlstr.toUtf8();
    auto doc = xmlParseMemory(u8.data(), u8.size());

    if (!doc) {
        auto err = xmlGetLastError();
        ZOOD_CLOG("Failed to Parse danmaku %s", err->message);
        return std::nullopt;
    }
    xmlNodePtr cur = xmlDocGetRootElement(doc);

    // For each children
    DanmakuList danmakus;

    for (cur = cur->children; cur; cur = cur->next) {
        if (xmlStrcmp(cur->name, BAD_CAST "d")) {
            continue;
        }

        // Get attr
        auto p = xmlGetProp(cur, BAD_CAST "p");
        auto text = xmlNodeGetContent(cur);
        auto list = QString::fromUtf8((const char*)p).split(",");
        bool ok = true;

        DanmakuItem d;
        // auto res = std::from_chars(list[0].data(), list[0].data() + list[0].size(), d.position);
        d.position = list[0].toDouble(&ok);
        if (!ok) {
            return std::nullopt;
        }

        // res = std::from_chars(list[1].data(), list[1].data() + list[1].size(), (int &)d.type);
        d.type =  (DanmakuItem::Type) list[1].toInt(&ok);
        if (!ok) {
            return std::nullopt;
        }


        // res = std::from_chars(list[2].data(), list[2].data() + list[2].size(), (int&)d.size);
        d.size = (DanmakuItem::Size) list[2].toInt(&ok);
        if (!ok) {
            return std::nullopt;
        }
        // int color_num;
        // res = std::from_chars(list[3].data(), list[3].data() + list[3].size(), color_num);
        // int r = 
        //     (color_num & 0xFF0000) >> 16;
        // int g = 
        //     (color_num & 0xFF00) >> 8;
        // int b = 
        //     (color_num & 0xFF);

        int color_num = list[3].toInt(&ok);
        if (!ok) {
            return std::nullopt;
        }

        d.color.setRgb(color_num);
        
        // d.color = Color(r, g, b);
        // d.shadow = Color::Gray;

        // res = std::from_chars(list[5].data(), list[5].data() + list[5].size(), (int &)d.pool);
        d.pool = (DanmakuItem::Pool) list[5].toInt(&ok);
        if (!ok) {
            return std::nullopt;
        }

        // res = std::from_chars(list[8].data(), list[8].data() + list[8].size(), (int &)d.level);
        d.level = list[8].toInt(&ok);
        if (!ok) {
            return std::nullopt;
        }


        d.text = reinterpret_cast<const char*>(text);


        danmakus.push_back(d);
    }

    // Sort danmaku by position
    SortDanmaku(&danmakus);

    xmlFreeDoc(doc);

    return danmakus;

}
DanmakuList MergeDanmaku(const DanmakuList &a, const DanmakuList &b) {
    DanmakuList result;

    result.append(a);
    result.append(b);

    SortDanmaku(&result);

    return result;
}
void       SortDanmaku(DanmakuList *d) {
    if (!d) {
        return;
    }
    std::sort(d->begin(), d->end(), [](const DanmakuItem &a,const DanmakuItem &b){
        return a.position < b.position;
    });
}