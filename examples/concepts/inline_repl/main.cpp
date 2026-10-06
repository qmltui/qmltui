#include <QCoreApplication>
#include <QQmlApplicationEngine>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
#ifdef QTUI_INLINE_REPL_SOURCE
    engine.load(QUrl::fromLocalFile(QStringLiteral(QTUI_INLINE_REPL_SOURCE)));
#else
    engine.loadFromModule("InlineRepl", "Main");
#endif
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}
