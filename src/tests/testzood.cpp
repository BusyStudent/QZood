#include <random>
#include <QApplication>

#include "../ui/zood/zood.hpp"
#include "testregister.hpp"
#include "../net/bilibili.hpp"
#include "../ui/zood/homeWidget.hpp"
#include "../BLL/data/videoSourceBLL.hpp"

ZOOD_TEST_W(Ui, zoodTest) {
	Zood* zood = new Zood();

    BiliClient* client = new BiliClient(zood);

    QWidget::connect(zood, &Zood::editTextChanged, client,[client,zood, old_client = NetResult<QStringList>(), old_text = QString()](const QString &text) mutable {
        if (!text.isEmpty() && text != old_text) {
            old_text = text;
            old_client.cancel();
            old_client = client->fetchSearchSuggestions(text).then([zood](const Result<QStringList> &textList){
                if (textList.has_value()) {
                    zood->setPredictStringList(textList.value());
                }
            });
        }
    });

    srand((unsigned)time(NULL));

    QWidget::connect(zood->homeWidget(), &HomeWidget::dataRequest, zood, [zood](HomeWidget::DisplayArea area) {
        auto video_views = zood->homeWidget()->addItems(area, rand() % 10 + 1);
        for (auto &video_view : video_views) {
            video_view->setTitle("我们仍未知道那天盛开的花的名字");
            video_view->setExtraInformation("这是额外信息，这是一条非常非常非常非常非常非常长的信息。");
            video_view->setSourceInformation("b站，樱花，异世界。");
            video_view->setImage(QImage(":/icons/loading_bar.png"));
        }
    });

    return zood;
}

ZOOD_TEST_C(ZOOD, cmdTestPassed) {
    EXPECT_EQ(10, 10) << "正确";
    EXPECT_LT(12, 13) << "正确";
    EXPECT_LE(13, 13) << "正确";
    EXPECT_GT(13, 12) << "正确";
    EXPECT_GE(13, 13) << "正确";
    EXPECT_TRUE(1 + 1 == 2) << "正确";
    EXPECT_FALSE(1 + 1 == 3) << "正确";
}

ZOOD_TEST_C(ZOOD, cmdTestFailed) {
    EXPECT_EQ(10, 9) << "10不等于9";
    EXPECT_LT(12, 12) << "12等于12";
    EXPECT_LE(13, 11) << "13大于11";
    EXPECT_GT(13, 13) << "13等于13";
    EXPECT_GE(13, 14) << "13小于14";
    EXPECT_TRUE(1 + 1 == 3) << "1+1等于2";
    EXPECT_FALSE(1 + 1 == 2) << "1+1等于2";
}

ZOOD_TEST_C(ZOOD, VideoSourceBLLTest) {
    QMetaObject::invokeMethod(
        qApp,
        [id](){
            VideoSourceBLL &video = VideoSourceBLL::instance();
            video.searchSuggestion("刀剑", nullptr, [id](const Result<QStringList> &list){
                if (!list.has_value()) {
                    qWarning() << "没有搜索到搜索建议";
                    return;
                }
                for (const QString& title : list.value()) {
                    ZoodLogString(title);
                }
            });
            video.searchBangumiFromText("刀剑", nullptr, [id](const Result<BangumiList>& bangumiList) {
                if (!bangumiList.has_value()) {
                    qWarning() << "没有搜索到对应的番剧";
                    return;
                }
                for (const auto& bangumi : bangumiList.value()) {
                    ZoodLogString(bangumi->rootInterface()->name());
                    ZoodLogString(bangumi->title());
                    ZoodLogString(bangumi->description());
                }
            });
            video.searchVideoTimeline(TimeWeek::MONDAY, nullptr, [id, &video](const Result<Timeline> &timeline) {
                if (!timeline.has_value()) {
                    video.searchBangumiFromTimeline(timeline.value(), nullptr, [id](const Result<BangumiList>& bangumis) {
                        if (bangumis.has_value()) {
                            for (auto& bangumi : bangumis.value()) {
                                ZoodLogString(bangumi->rootInterface()->name());
                                ZoodLogString(bangumi->title());
                                ZoodLogString(bangumi->description());
                            }
                        }
                    });
                }
            });
        },
        Qt::QueuedConnection);
}