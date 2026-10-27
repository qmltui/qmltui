#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTextStream>
#include <QVariantMap>

static QString normalizeKey(QString name) {
    if (name.endsWith(QStringLiteral(".qml"), Qt::CaseInsensitive)) name.chop(4);
    name.remove(QLatin1Char('_'));
    name.remove(QLatin1Char('-'));
    return name.toLower();
}

static QStringList availableExamples() {
    const QDir dir(QStringLiteral(":/qt/qml/Gallery/examples"));
    return dir.entryList({QStringLiteral("*.qml")}, QDir::Files, QDir::Name);
}

static QVariantMap loadExampleSources(const QStringList& files) {
    QVariantMap sources;
    for (const QString& fileName : files) {
        QFile file(QStringLiteral(":/qt/qml/Gallery/examples/") + fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            sources.insert(fileName, QString::fromUtf8(file.readAll()));
        }
    }
    return sources;
}

static QVariantMap loadScenarioSources() {
    QVariantMap sources;
    const QDir dir(QStringLiteral(":/qt/qml/Gallery/scenarios"));
    const QStringList files = dir.entryList({QStringLiteral("*.js")}, QDir::Files, QDir::Name);
    for (const QString& fileName : files) {
        QFile file(QStringLiteral(":/qt/qml/Gallery/scenarios/") + fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            sources.insert(fileName, QString::fromUtf8(file.readAll()));
        }
    }
    return sources;
}

static QString resolveExampleFile(const QString& query, const QStringList& files) {
    if (query.isEmpty()) return QString();
    const QString targetKey = normalizeKey(query);
    for (const QString& file : files) {
        if (file.compare(query, Qt::CaseInsensitive) == 0 || normalizeKey(file) == targetKey)
            return file;
    }
    return QString();
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    const QStringList examples = availableExamples();
    QString requestedExample = qEnvironmentVariable("QMLTUI_GALLERY_EXAMPLE");
    QString requestedPage = qEnvironmentVariable("QMLTUI_GALLERY_PAGE");
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString& arg = args.at(i);
        if (arg == QStringLiteral("--list-examples")) {
            QTextStream out(stdout);
            for (const QString& file : examples) {
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
    engine.rootContext()->setContextProperty(QStringLiteral("qmltuiAvailableExamples"), examples);
    engine.rootContext()->setContextProperty(QStringLiteral("qmltuiExampleSources"),
                                             loadExampleSources(examples));
    engine.rootContext()->setContextProperty(QStringLiteral("qmltuiScenarioSources"),
                                             loadScenarioSources());

    if (!requestedExample.isEmpty()) {
        const QString resolved = resolveExampleFile(requestedExample, examples);
        if (resolved.isEmpty()) {
            QTextStream(stderr) << "Unknown gallery example: " << requestedExample << Qt::endl;
            return 2;
        }
        engine.rootContext()->setContextProperty(QStringLiteral("qmltuiStandaloneExample"),
                                                 resolved);
    } else if (!requestedPage.isEmpty()) {
        const QString resolved = resolveExampleFile(requestedPage, examples);
        if (!resolved.isEmpty()) {
            engine.rootContext()->setContextProperty(QStringLiteral("qmltuiGalleryPage"), resolved);
        }
    }

    engine.loadFromModule("Gallery", "Main");
    if (engine.rootObjects().isEmpty()) return -1;
    return app.exec();
}
