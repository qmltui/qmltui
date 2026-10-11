// Loads one QML file whose root is a Terminal and hands it to Runner's
// environment-driven startup. scripts/check_doc_examples.py runs every ```qml
// block from docs/components through it with QTUI_RUNNER_CAPTURE_AFTER_MS set, so a
// documentation example that fails to load or render fails the suite.
#include <QCoreApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QUrl>

#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Runner/Runner.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    if (argc < 2) {
        qCritical() << "usage: qtui_doc_example_runner <Main.qml>";
        return 2;
    }
    QQmlApplicationEngine engine;
    engine.load(QUrl::fromLocalFile(QString::fromUtf8(argv[1])));
    if (engine.rootObjects().isEmpty())
        return 3;   // the QML engine already printed the load errors
    auto* terminal = qobject_cast<qtui::Terminal*>(engine.rootObjects().constFirst());
    if (!terminal) {
        qCritical() << "the example's root object is not a Terminal";
        return 4;
    }
    if (!qtui::Runner::startFromEnvironment(&engine, terminal)) {
        qCritical() << "set QTUI_RUNNER_CAPTURE_AFTER_MS (or QTUI_RUNNER_SCRIPT / QTUI_RUNNER_EVAL)";
        return 2;
    }
    return app.exec();
}
