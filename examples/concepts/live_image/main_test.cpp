#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <Qtui/Image/ImageProvider.h>

#include "frameprovider.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    // Shape 2: pictures under image://frames/... come from this asynchronous
    // provider, rendered on its own thread; the engine owns it. Shape 1
    // (FrameSource) is a QML type, see Main.qml.
    qtui::ImageProvider::add(&engine, QStringLiteral("frames"), new RadarProvider);
    engine.loadFromModule("LiveImageTest", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
