#include <QApplication>
#include <QSurfaceFormat>

#include "./ui/zood/zood.hpp"

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

    Zood zood;
    zood.show();
    return a.exec();
}