#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QCoreApplication application(argc, argv);
    QQmlApplicationEngine engine;
#ifdef QTUI_TERMINAL_VIEW_SOURCE
    engine.load(QUrl::fromLocalFile(QStringLiteral(QTUI_TERMINAL_VIEW_SOURCE)));
#else
    engine.loadFromModule("TerminalViewDemo", "Main");
#endif
    if (engine.rootObjects().isEmpty())
        return -1;
    return application.exec();
}
