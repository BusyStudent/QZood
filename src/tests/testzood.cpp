#include "../ui/zood/zood.hpp"
#include "testwindow.hpp"
#include "../net/bilibili.hpp"

ZOOD_TEST(zoodTest) {
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