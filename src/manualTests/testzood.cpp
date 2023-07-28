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

    return zood;
}