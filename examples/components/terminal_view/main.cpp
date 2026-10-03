#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[]) {
    QCoreApplication application(argc, argv);
    QQmlApplicationEngine engine;
    engine.loadFromModule("TerminalViewDemo", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return application.exec();
}

