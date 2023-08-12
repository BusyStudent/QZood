#include <QApplication>
#include <QMainWindow>
#include <QSurfaceFormat>
#include <QThreadPool>

#include "testwindow.hpp"
#include "../ui/util/widget/customizeTitleWidget.hpp"
#include "../ui/zood/zood.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <QWindow>
#include <QListWidget>
#include <QHBoxLayout>
#include <windows.h>
#pragma comment(linker, "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
static void OpenTerminal(QWidget *parent)
{    
    // AllocConsole();
    // freopen("CON","w",stdout);
    // freopen("CON","w",stderr);
    // freopen("CON","r",stdin);
    // auto hwnd = GetConsoleWindow();
    // auto window = QWindow::fromWinId((WId)hwnd);
    // auto widget = QWidget::createWindowContainer(window, nullptr);
    // parent->setContentsMargins(0, 0, 0, 0);
    // auto hboxlayout = new QHBoxLayout();
    // parent->setLayout(hboxlayout);
    // hboxlayout->setContentsMargins(0, 0, 0, 0);
    // hboxlayout->addWidget(widget);
}
#else
void OpenTerminal(QWidget *parent) { }
#endif

static void SetDefaultFormat() {
    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(format);
}

int main(int argc, char **argv) {

#if !defined(_WIN32) || !defined(ZOOD_WEBENGINE_CORE)
    SetDefaultFormat();
#endif

    QApplication a(argc, argv);

#if defined(_WIN32) && defined(ZOOD_WEBENGINE_CORE)
    SetDefaultFormat();
#endif

    QThreadPool::globalInstance()->setMaxThreadCount(4);

#if !defined(NDEBUG)
    ZoodTestWindow twin;
    OpenTerminal(twin.TerminatorParent());
    twin.show();
#else
    Zood zood;
    zood.show();
#endif

    return a.exec();
}