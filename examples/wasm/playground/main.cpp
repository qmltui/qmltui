// Qamui QML playground for the browser: one binary, QML source supplied by the
// page at run time. The page calls Module.qamuiLoadQml(source, fileName) after
// every edit; the previous root is destroyed and the new one instantiated.
// No user C++ is possible here by design; everything Qamui.* offers is linked.

#include <QCoreApplication>
#include <QEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlError>
#include <QRegularExpression>
#include <QUrl>

#include <emscripten.h>
#include <emscripten/bind.h>

#include <string>
#include <utility>

namespace {

QQmlEngine* g_engine = nullptr;
QObject* g_root = nullptr;

// Page callbacks; each is optional.
EM_JS(void, playground_js_quit, (), {
    if (Module['qamuiOnQuit']) Module['qamuiOnQuit']();
});
EM_JS(void, playground_js_warning, (const char* text, int length), {
    if (Module['qamuiOnWarning']) Module['qamuiOnWarning'](UTF8ToString(text, length));
});

std::string errorsToJson(const QList<QQmlError>& errors)
{
    QJsonArray array;
    for (const QQmlError& error : errors) {
        QJsonObject item;
        item[QStringLiteral("line")] = error.line();
        item[QStringLiteral("column")] = error.column();
        item[QStringLiteral("message")] = error.description();
        item[QStringLiteral("file")] = error.url().fileName();
        array.append(item);
    }
    return QJsonDocument(array).toJson(QJsonDocument::Compact).toStdString();
}

// Returns "" on success, otherwise a JSON array of {line, column, message, file}.
std::string loadQml(const std::string& source, const std::string& fileName)
{
    if (!g_engine)
        return R"([{"line":0,"column":0,"message":"engine not ready","file":""}])";

    // Tear down the previous Terminal first, so its shutdown output (leave the
    // alternate screen, show the cursor) precedes the new root's first frame.
    // A root released by Qt.quit() is pending in deleteLater(); flush it now.
    delete g_root;
    g_root = nullptr;
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    g_engine->clearComponentCache();

    // file:///playground/ is Emscripten's in-memory filesystem; the page may
    // write sibling files there so relative imports resolve.
    const QUrl url(QStringLiteral("file:///playground/") + QString::fromStdString(fileName));
    QQmlComponent component(g_engine);
    component.setData(QByteArray::fromStdString(source), url);
    if (component.isError())
        return errorsToJson(component.errors());

    g_root = component.create();
    if (!g_root)
        return errorsToJson(component.errors());

    if (!g_root->inherits("qamui::Terminal")) {
        QString name = QString::fromLatin1(g_root->metaObject()->className());
        name.remove(QRegularExpression(QStringLiteral("_QMLTYPE_\\d+$")));   // Main_QMLTYPE_4 -> Main
        delete g_root;
        g_root = nullptr;
        QJsonObject item;
        item[QStringLiteral("line")] = 1;
        item[QStringLiteral("column")] = 1;
        item[QStringLiteral("message")] = QStringLiteral("the root item must be a Terminal, not %1").arg(name);
        item[QStringLiteral("file")] = QString::fromStdString(fileName);
        return QJsonDocument(QJsonArray{item}).toJson(QJsonDocument::Compact).toStdString();
    }
    return {};
}

} // namespace

// Compiles `source` without creating anything: "" or the same JSON error
// array as loadQml. The page checks an edited example this way before it is
// handed to the running gallery, which reports only what it can catch.
std::string checkQml(const std::string& source, const std::string& fileName)
{
    if (!g_engine)
        return R"([{"line":0,"column":0,"message":"engine not ready","file":""}])";
    QQmlComponent component(g_engine);
    component.setData(QByteArray::fromStdString(source),
                      QUrl(QStringLiteral("file:///playground/") + QString::fromStdString(fileName)));
    return component.isError() ? errorsToJson(component.errors()) : std::string();
}

// Page-defined state the next root can read as a context property, e.g. the
// gallery's qamuiGalleryPage. Applies to roots created after the call.
void setContextProperty(const std::string& name, const std::string& value)
{
    if (g_engine)
        g_engine->rootContext()->setContextProperty(QString::fromStdString(name), QString::fromStdString(value));
}

EMSCRIPTEN_BINDINGS(qamui_playground)
{
    emscripten::function("qamuiLoadQml", &loadQml);
    emscripten::function("qamuiCheckQml", &checkQml);
    emscripten::function("qamuiSetContextProperty", &setContextProperty);
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QQmlEngine engine;
    g_engine = &engine;
    engine.addImportPath(QStringLiteral("qrc:/qt/qml"));
    engine.setOutputWarningsToStandardError(false);
    QObject::connect(&engine, &QQmlEngine::warnings, [](const QList<QQmlError>& warnings) {
        for (const QQmlError& warning : warnings) {
            const std::string text = warning.toString().toStdString();
            playground_js_warning(text.data(), static_cast<int>(text.size()));
        }
    });
    // Qt.quit() from user QML must not end the runtime (Qt would abort without
    // Asyncify); tell the page instead and leave the engine ready for the next edit.
    // The root is released with deleteLater(): quit() arrives from inside one
    // of the root's own signal handlers, and deleting it there aborts.
    const auto releaseRoot = [] {
        if (QObject* root = std::exchange(g_root, nullptr))
            root->deleteLater();
        playground_js_quit();
    };
    QObject::connect(&engine, &QQmlEngine::quit, releaseRoot);
    QObject::connect(&engine, &QQmlEngine::exit, [releaseRoot](int) { releaseRoot(); });
    return app.exec();   // never returns in the browser; the page drives qamuiLoadQml
}
