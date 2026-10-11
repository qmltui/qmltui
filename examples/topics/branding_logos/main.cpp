#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QString>

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(branding);

    QCoreApplication app(argc, argv);
    app.setApplicationName("qamui_branding_logos");

    int initialIndex = 0;
    if (argc > 1) {
        bool ok = false;
        int parsed = QString::fromUtf8(argv[1]).toInt(&ok);
        if (ok && parsed >= 0) {
            initialIndex = parsed;
        }
    }

    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("initialIndex"), initialIndex}});
    engine.loadFromModule("BrandingLogosDemo", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
