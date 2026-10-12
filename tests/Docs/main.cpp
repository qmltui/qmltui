// Loads one QML file whose root is a Terminal and hands it to Runner's
// environment-driven startup. scripts/check_doc_examples.py runs every ```qml
// block from docs/qml-types through it with QAMUI_RUNNER_CAPTURE_AFTER_MS set, so a
// documentation example that fails to load or render fails the suite.
#include <QCoreApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QUrl>

#include <Qamui/Core/Items/Terminal.h>
#include <Qamui/Runner/Runner.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        qCritical() << "usage: qamui_doc_example_runner <Main.qml>";
        return 2;
    }
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(QString::fromUtf8(argv[1])));
    if (engine.rootObjects().isEmpty())
        return 3;   // the QML engine already printed the load errors
    auto* terminal = qobject_cast<qamui::Terminal*>(engine.rootObjects().constFirst());
    if (!terminal) {
        qCritical() << "the example's root object is not a Terminal";
        return 4;
    }
    if (!qamui::Runner::startFromEnvironment(&engine, terminal)) {
        qCritical() << "set QAMUI_RUNNER_CAPTURE_AFTER_MS (or QAMUI_RUNNER_SCRIPT / QAMUI_RUNNER_EVAL)";
        return 2;
    }
    return app.exec();
}
