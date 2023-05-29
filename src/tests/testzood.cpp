#include <random>

#include "../ui/zood/zood.hpp"
#include "testregister.hpp"
#include "../net/bilibili.hpp"

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
    QWidget::connect(zood->homeWidget(), &HomeWidget::refreshRequest, zood, [zood](HomeWidget::DisplayArea area){
        auto video_views = zood->homeWidget()->addItems(area, rand() % 10 + 1);
        for (auto &video_view : video_views) {
            video_view->setVideoId(1);
            video_view->setTitle("刀剑神域");
            video_view->setExtraInformation("这是额外信息，这是一条非常非常非常非常非常非常长的信息。");
            video_view->setSourceInformation("b站，樱花，异世界。");
            video_view->setImage(QImage(":/icons/loading_bar.png"));
        }
    });

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
    EXPECT_EQ(10, 10);
    EXPECT_LT(12, 13);
    EXPECT_LE(13, 13);
    EXPECT_GT(13, 12);
    EXPECT_GE(13, 13);
    EXPECT_TRUE(1 + 1 == 2);
    EXPECT_FALSE(1 + 1 == 3);
}

ZOOD_TEST_C(ZOOD, cmdTestFailed) {
    EXPECT_EQ(10, 9);
    EXPECT_LT(12, 12);
    EXPECT_LE(13, 11);
    EXPECT_GT(13, 13);
    EXPECT_GE(13, 14);
    EXPECT_TRUE(1 + 1 == 3);
    EXPECT_FALSE(1 + 1 == 2);
}