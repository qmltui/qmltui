#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTextStream>
#include <QUrl>

static QString normalizeKey(QString name) {
    if (name.endsWith(QStringLiteral(".qml"), Qt::CaseInsensitive)) name.chop(4);
    name.remove(QLatin1Char('_'));
    name.remove(QLatin1Char('-'));
    return name.toLower();
}

static QString resolveExampleFile(const QString& query) {
    if (query.isEmpty()) return QString();
    const QDir dir(QStringLiteral(":/gallery/examples"));
    const QStringList files = dir.entryList({QStringLiteral("*.qml")}, QDir::Files, QDir::Name);
    const QString targetKey = normalizeKey(query);
    for (const QString& file : files) {
        if (file.compare(query, Qt::CaseInsensitive) == 0 || normalizeKey(file) == targetKey)
            return file;
    }
    return QString();
}

// The launcher instantiates each demo from its source text, so hand it every
// file under examples/ as one JSON object { "Button.qml": "<qml>", ... }. The
// wasm page passes the same shape from its editor.
static QString exampleSources() {
    QJsonObject sources;
    const QDir dir(QStringLiteral(":/gallery/examples"));
    for (const QString& name : dir.entryList({QStringLiteral("*.qml")}, QDir::Files, QDir::Name)) {
        QFile file(dir.filePath(name));
        if (file.open(QIODevice::ReadOnly)) sources[name] = QString::fromUtf8(file.readAll());
    }
    return QString::fromUtf8(QJsonDocument(sources).toJson(QJsonDocument::Compact));
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    QString requestedExample = qEnvironmentVariable("QMLTUI_GALLERY_EXAMPLE");
    QString requestedPage = qEnvironmentVariable("QMLTUI_GALLERY_PAGE");
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString& arg = args.at(i);
        if (arg == QStringLiteral("--list-examples")) {
            QTextStream out(stdout);
            const QDir dir(QStringLiteral(":/gallery/examples"));
            for (const QString& file :
                 dir.entryList({QStringLiteral("*.qml")}, QDir::Files, QDir::Name)) {
                out << file.left(file.size() - 4) << Qt::endl;
            }
            return 0;
        }
        if (arg == QStringLiteral("--example") && i + 1 < args.size()) {
            requestedExample = args.at(++i);
        } else if (arg.startsWith(QStringLiteral("--example="))) {
            requestedExample = arg.mid(10);
        } else if (arg == QStringLiteral("--page") && i + 1 < args.size()) {
            requestedPage = args.at(++i);
        } else if (arg.startsWith(QStringLiteral("--page="))) {
            requestedPage = arg.mid(7);
        }
    }

    QQmlApplicationEngine engine;
    if (!requestedExample.isEmpty()) {
        const QString resolved = resolveExampleFile(requestedExample);
        if (resolved.isEmpty()) {
            QTextStream(stderr) << "Unknown gallery example: " << requestedExample << Qt::endl;
            return 2;
        }
        engine.load(QUrl(QStringLiteral("qrc:/gallery/examples/") + resolved));
        if (engine.rootObjects().isEmpty()) return -1;
        return app.exec();
    }

    engine.rootContext()->setContextProperty(QStringLiteral("qmltuiExampleSources"),
                                             exampleSources());
    if (!requestedPage.isEmpty()) {
        const QString resolved = resolveExampleFile(requestedPage);
        if (!resolved.isEmpty()) {
            engine.rootContext()->setContextProperty(QStringLiteral("qmltuiGalleryPage"), resolved);
        }
    }
    engine.loadFromModule("Gallery", "Main");
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}
