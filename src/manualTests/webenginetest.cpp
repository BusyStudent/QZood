#if 0

#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineScriptCollection>
#include <QWebEngineScript>
#include <QWebEnginePage>
#include <QWebChannel>
#include <QWidget>
#include <QHBoxLayout>
#include "testregister.hpp"
#include "ui_webenginetest.h"


ZOOD_TEST(WebEngine, Userscript) {
    class MyPage : public QWebEnginePage {
        public:
            using QWebEnginePage::QWebEnginePage;

            void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level,
                                          const QString &message, int lineNumber,
                                          const QString &sourceID) override {
                auto item = new QListWidgetItem(message);
                switch (level) {
                    case WarningMessageLevel: {
                        item->setBackground(Qt::yellow);
                        break;
                    }
                    case ErrorMessageLevel: {
                        item->setBackground(QColor("#FFC0CB"));
                        item->setForeground(Qt::red);
                        break;
                    }
                }
                s.infoWidget->addItem(item);
            }

            Ui::WebScript s;
    };

    auto widget = new QWidget();
    auto page = new MyPage(widget);
    auto channel = new QWebChannel(widget);

    page->setWebChannel(channel);

    Ui::WebScript s;
    s.setupUi(widget);

    page->s = s;

    QObject::connect(page, &QWebEnginePage::urlChanged, [=](const QUrl &url) {
        s.urlEdit->setText(url.toString());
    });
    QObject::connect(page, &QWebEnginePage::loadFinished, [=](bool v) {
        page->toPlainText([=](const QString &what) {
            s.htmlBroswer->setPlainText(what);
        });
    });

    QObject::connect(s.refreshButton, &QPushButton::clicked, [=]() {
        page->triggerAction(QWebEnginePage::Reload);
    });
    QObject::connect(s.stopButton, &QPushButton::clicked, [=]() {
        page->triggerAction(QWebEnginePage::Stop);
    });
    QObject::connect(s.urlEdit, &QLineEdit::returnPressed, [=]() {
        s.infoWidget->clear();
        page->load(s.urlEdit->text());
    });
    

    // Systems
    class MyWatcher : public QWebEngineUrlRequestInterceptor {
        public:
            using QWebEngineUrlRequestInterceptor::QWebEngineUrlRequestInterceptor;

            void interceptRequest(QWebEngineUrlRequestInfo &info) override {
                s.infoWidget->addItem(QString("%1 %2").arg(info.requestMethod(), info.requestUrl().toString()));
            }
            Ui::WebScript s;
    };
    auto watcher = new MyWatcher(widget);
    page->setUrlRequestInterceptor(watcher);
    watcher->s = s;


    // Script
    QObject::connect(s.evalButton, &QPushButton::clicked, [=]() mutable {
        auto code = s.scriptEdit->toPlainText();
        page->runJavaScript(code, [=, n = 0](const QVariant &v) mutable {
            auto inItem = new QListWidgetItem(s.evalResultWidget);
            s.evalResultWidget->addItem(inItem);
            auto outItem = new QListWidgetItem(s.evalResultWidget);
            s.evalResultWidget->addItem(outItem);

            inItem->setText(QString("    In [%1] : ").arg(n) + code);
            inItem->setForeground(Qt::blue);

            QString msg;
            QDebug d(&msg);
            d << v;

            outItem->setText(QString("    Out[%1] : ").arg(n) + msg);
            outItem->setForeground(Qt::red);

            n += 1;
        });
    });

    // Userscript
    QObject::connect(s.injectButton, &QPushButton::clicked, [=]() {
        QWebEngineScript script;
        script.setSourceCode(s.userscriptEdit->toPlainText());
        script.setInjectionPoint(QWebEngineScript::DocumentCreation);
        script.setRunsOnSubFrames(true);
        script.setWorldId(QWebEngineScript::MainWorld);

        QWebEngineScript ascript;
        ascript.setSourceCode(s.userscriptEdit->toPlainText());
        ascript.setInjectionPoint(QWebEngineScript::DocumentCreation);
        ascript.setRunsOnSubFrames(true);
        ascript.setWorldId(QWebEngineScript::ApplicationWorld);

        auto &col = page->scripts();
        col.clear();
        col.insert(script);
        col.insert(ascript);
    });


    return widget;
}

#endif