#include "../ui/zood/zood.hpp"
#include "testwindow.hpp"
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