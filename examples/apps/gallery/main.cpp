#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>

// The launcher instantiates each demo from its source text, so hand it every
// file under examples/ as one JSON object { "Button.qml": "<qml>", ... }. The
// wasm page passes the same shape from its editor.
static QString exampleSources()
{
    QJsonObject sources;
    const QDir dir(QStringLiteral(":/gallery/examples"));
    for (const QString& name : dir.entryList({QStringLiteral("*.qml")}, QDir::Files, QDir::Name)) {
        QFile file(dir.filePath(name));
        if (file.open(QIODevice::ReadOnly))
            sources[name] = QString::fromUtf8(file.readAll());
    }
    return QString::fromUtf8(QJsonDocument(sources).toJson(QJsonDocument::Compact));
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("qamuiExampleSources"), exampleSources());
    engine.loadFromModule("Gallery", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;
    return app.exec();
}
