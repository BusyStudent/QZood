#include <QApplication>
#include <QMainWindow>
#include <QSurfaceFormat>
#include <QThreadPool>

#include "tests/testwindow.hpp"
#include "ui/common/customizeTitleWidget.hpp"
#include "ui/zood/zood.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <QWindow>
#include <QListWidget>
#include <QHBoxLayout>
static void OpenTerminal(QWidget *parent)
{    
    AllocConsole();
    freopen("CON","w",stdout);
    freopen("CON","w",stderr);
    freopen("CON","r",stdin);
    auto hwnd = GetConsoleWindow();
    auto window = QWindow::fromWinId((WId)hwnd);
    auto widget = QWidget::createWindowContainer(window, nullptr);
    parent->setContentsMargins(0, 0, 0, 0);
    auto hboxlayout = new QHBoxLayout();
    parent->setLayout(hboxlayout);
    hboxlayout->setContentsMargins(0, 0, 0, 0);
    hboxlayout->addWidget(widget);
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

#if !defined(_WIN32)
    SetDefaultFormat();
#endif

    QApplication a(argc, argv);

#if defined(_WIN32)
    SetDefaultFormat();
#endif

    QThreadPool::globalInstance()->setMaxThreadCount(4);

    ZoodTestWindow* twin = new ZoodTestWindow();
    twin->setAttribute(Qt::WA_DeleteOnClose, true);
    OpenTerminal(twin->TerminatorParent());
    twin->show();

    return a.exec();
}