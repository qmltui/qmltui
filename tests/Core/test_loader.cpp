#include <QFile>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>

#include <memory>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Loader.h>
#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include "../TestPaint.h"

using namespace qmltui;
using qmltui::testing::paintItem;

class TestLoader : public QObject {
    Q_OBJECT

private slots:
    void loadsInlineComponent();
    void loadsRelativeUrl();
    void loadsNonVisualObject();
    void leavesNonVisualObjectUnsized();
    void activeControlsLifetime();
    void replacingComponentDestroysItem();
    void followsLoadedImplicitSize();
    void propagatesExplicitSize();
    void reportsErrorOnInvalidSource();
};

static void addQmlTuiImportPath(QQmlEngine& engine) {
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../QmlTui"));
}

void TestLoader::loadsInlineComponent() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        Loader {
            sourceComponent: Text {
                objectName: "content"
                text: "loaded"
            }
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* loader = qobject_cast<Loader*>(object.get());
    QVERIFY(loader);
    QCOMPARE(loader->status(), Loader::Ready);
    QVERIFY(loader->item());
    QCOMPARE(loader->item()->objectName(), QStringLiteral("content"));
    auto* content = qobject_cast<Item*>(loader->item());
    QVERIFY(content);
    QCOMPARE(content->parentItem(), loader);
    QCOMPARE(loader->implicitWidth(), 6);
    QCOMPARE(loader->implicitHeight(), 1);

    TextGridBuffer buffer;
    buffer.resize(10, 1);
    paintItem(loader, buffer, 0, 0);
    QString rendered;
    for (int x = 0; x < 6; ++x)
        rendered += QString::fromUtf8(buffer.cellAt(x, 0).character());
    QCOMPARE(rendered, QStringLiteral("loaded"));
}

void TestLoader::loadsRelativeUrl() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QFile sourceFile(directory.filePath(QStringLiteral("Loaded.qml")));
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    const QByteArray sourceData = R"(
        import QmlTui
        Item { objectName: "urlContent"; implicitWidth: 9 }
    )";
    QCOMPARE(sourceFile.write(sourceData), sourceData.size());
    sourceFile.close();

    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        Loader { source: "Loaded.qml" }
    )",
                      QUrl::fromLocalFile(directory.filePath(QStringLiteral("Host.qml"))));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* loader = qobject_cast<Loader*>(object.get());
    QVERIFY(loader);
    QCOMPARE(loader->status(), Loader::Ready);
    QVERIFY(loader->item());
    QCOMPARE(loader->item()->objectName(), QStringLiteral("urlContent"));
    QCOMPARE(loader->implicitWidth(), 9);
}

void TestLoader::loadsNonVisualObject() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        import QtQml
        Loader {
            sourceComponent: QtObject {
                objectName: "theme"
                property string accent: "cyan"
            }
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* loader = qobject_cast<Loader*>(object.get());
    QVERIFY(loader);
    QCOMPARE(loader->status(), Loader::Ready);
    QVERIFY(loader->errorString().isEmpty());

    QObject* loadedObject = loader->item();
    QVERIFY(loadedObject);
    QCOMPARE(loadedObject->objectName(), QStringLiteral("theme"));
    QVERIFY(!qobject_cast<Item*>(loadedObject));
    QCOMPARE(loadedObject->property("accent").toString(), QStringLiteral("cyan"));

    // The object is owned by the loader but is not part of its visual tree.
    QCOMPARE(loadedObject->parent(), loader);
    QVERIFY(loader->childItems().isEmpty());

    // No sizing rule applies to a non-visual source.
    QCOMPARE(loader->implicitWidth(), 0);
    QCOMPARE(loader->implicitHeight(), 0);

    QPointer<QObject> tracked = loadedObject;
    loader->setActive(false);
    QCOMPARE(loader->status(), Loader::Null);
    QVERIFY(!loader->item());
    QVERIFY(tracked.isNull());
}

void TestLoader::leavesNonVisualObjectUnsized() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        QtObject { objectName: "controller" }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    Loader loader;
    loader.setWidth(20);
    loader.setHeight(7);
    loader.setSourceComponent(&component);

    QCOMPARE(loader.status(), Loader::Ready);
    QVERIFY(loader.item());
    QCOMPARE(loader.width(), 20);
    QCOMPARE(loader.height(), 7);
    QCOMPARE(loader.implicitWidth(), 0);
    QCOMPARE(loader.implicitHeight(), 0);

    // Resizing after loading must not reach a non-visual object either.
    loader.setWidth(30);
    QCOMPARE(loader.item()->objectName(), QStringLiteral("controller"));
}

void TestLoader::activeControlsLifetime() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        Loader {
            active: false
            sourceComponent: Item { implicitWidth: 4; implicitHeight: 2 }
        }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* loader = qobject_cast<Loader*>(object.get());
    QVERIFY(loader);
    QCOMPARE(loader->status(), Loader::Null);
    QVERIFY(!loader->item());

    QSignalSpy loadedSpy(loader, &Loader::loaded);
    loader->setActive(true);
    QCOMPARE(loader->status(), Loader::Ready);
    QVERIFY(loader->item());
    QCOMPARE(loadedSpy.size(), 1);

    QPointer<QObject> loadedItem = loader->item();
    loader->setActive(false);
    QCOMPARE(loader->status(), Loader::Null);
    QVERIFY(!loader->item());
    QVERIFY(loadedItem.isNull());
}

void TestLoader::replacingComponentDestroysItem() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent first(&engine);
    first.setData(R"(
        import QmlTui
        Item { objectName: "first" }
    )",
                  QUrl());
    QVERIFY2(first.isReady(), qPrintable(first.errorString()));

    QQmlComponent second(&engine);
    second.setData(R"(
        import QmlTui
        Item { objectName: "second" }
    )",
                   QUrl());
    QVERIFY2(second.isReady(), qPrintable(second.errorString()));

    Loader loader;
    loader.setSourceComponent(&first);
    QCOMPARE(loader.status(), Loader::Ready);
    QPointer<QObject> firstItem = loader.item();

    loader.setSourceComponent(&second);
    QVERIFY(firstItem.isNull());
    QCOMPARE(loader.status(), Loader::Ready);
    QVERIFY(loader.item());
    QCOMPARE(loader.item()->objectName(), QStringLiteral("second"));
}

void TestLoader::followsLoadedImplicitSize() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        Item { implicitWidth: 3; implicitHeight: 2 }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    Loader loader;
    loader.setSourceComponent(&component);
    QCOMPARE(loader.implicitWidth(), 3);
    QCOMPARE(loader.implicitHeight(), 2);

    auto* loadedItem = qobject_cast<Item*>(loader.item());
    QVERIFY(loadedItem);
    loadedItem->setImplicitWidth(8);
    loadedItem->setImplicitHeight(5);
    QCOMPARE(loader.implicitWidth(), 8);
    QCOMPARE(loader.implicitHeight(), 5);
}

void TestLoader::propagatesExplicitSize() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        Item { implicitWidth: 3; implicitHeight: 2 }
    )",
                      QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    Loader loader;
    loader.setWidth(20);
    loader.setHeight(7);
    loader.setSourceComponent(&component);

    auto* loadedItem = qobject_cast<Item*>(loader.item());
    QVERIFY(loadedItem);
    QCOMPARE(loadedItem->width(), 20);
    QCOMPARE(loadedItem->height(), 7);
    QCOMPARE(loader.implicitWidth(), 3);
    QCOMPARE(loader.implicitHeight(), 2);
}

void TestLoader::reportsErrorOnInvalidSource() {
    QQmlEngine engine;
    addQmlTuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        Loader { source: "missing_component_file.qml" }
    )",
                      QUrl::fromLocalFile(QStringLiteral("/tmp/Host.qml")));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto* loader = qobject_cast<Loader*>(object.get());
    QVERIFY(loader);
    QCOMPARE(loader->status(), Loader::Error);
    QVERIFY(!loader->errorString().isEmpty());
    QVERIFY(!loader->item());
}

QTEST_MAIN(TestLoader)
#include "test_loader.moc"
