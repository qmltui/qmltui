#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QString>
#include <QVariantMap>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QString initialPath = "panda.png";
    if (argc > 1 && QString::fromUtf8(argv[1]).trimmed().length() > 0) {
        initialPath = QString::fromUtf8(argv[1]).trimmed();
    } else if (qEnvironmentVariableIsSet("IMAGE_PATH")) {
        initialPath = QString::fromUtf8(qgetenv("IMAGE_PATH")).trimmed();
    }

    QQmlApplicationEngine engine;
    engine.setInitialProperties({{QStringLiteral("userImagePath"), initialPath}});
    engine.loadFromModule("ImageDemo", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
