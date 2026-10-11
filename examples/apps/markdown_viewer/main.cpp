#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QFileInfo>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QDir>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("qamui_markdown_viewer");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Qamui Terminal Markdown Viewer powered by KDE md4qt");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "Markdown file to open (optional)");
    parser.process(app);

    QString initialFile;
    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty()) {
        QString path = positionalArgs.first();
        QFileInfo fi(path);
        if (fi.exists()) {
            initialFile = fi.absoluteFilePath();
        } else {
            initialFile = path;
        }
    }

    QQmlApplicationEngine engine;

    if (!initialFile.isEmpty()) {
        engine.rootContext()->setContextProperty("initialCliFile", initialFile);
    } else {
        engine.rootContext()->setContextProperty("initialCliFile", QString());
    }

    engine.loadFromModule("Qamui.Examples.MarkdownViewer", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
