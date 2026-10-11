#include <QCoreApplication>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QTextStream>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    QLockFile instanceLock(QStringLiteral("/run/qamui_starter_template.lock"));
    instanceLock.setStaleLockTime(0);
    if (!instanceLock.tryLock()) {
        QTextStream(stderr) << "qamui_starter_template is already running\n";
        return 2;
    }

    QQmlApplicationEngine engine;
    engine.loadFromModule("StarterTemplateApp", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
