#include <QFile>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QQmlComponent>
#include <QQmlEngine>

#include <memory>

#include "Item.h"
#include "Loader.h"
#include "Text.h"

using namespace qtui;

class TestLoader : public QObject {
    Q_OBJECT

private slots:
    void loadsInlineComponent();
    void loadsRelativeUrl();
    void activeControlsLifetime();
    void replacingComponentDestroysItem();
    void followsLoadedImplicitSize();
    void propagatesExplicitSize();
};

static void addQtuiImportPath(QQmlEngine &engine) {
    engine.addImportPath(QCoreApplication::applicationDirPath()
                         + QStringLiteral("/../Qtui"));
}

void TestLoader::loadsInlineComponent() {
    QQmlEngine engine;
    addQtuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        Loader {
            sourceComponent: Text {
                objectName: "content"
                text: "loaded"
            }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *loader = qobject_cast<Loader *>(object.get());
    QVERIFY(loader);
    QCOMPARE(loader->status(), Loader::Ready);
    QVERIFY(loader->item());
    QCOMPARE(loader->item()->objectName(), QStringLiteral("content"));
    QCOMPARE(loader->item()->parentItem(), loader);
    QCOMPARE(loader->implicitWidth(), 6);
    QCOMPARE(loader->implicitHeight(), 1);
}

void TestLoader::loadsRelativeUrl() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    QFile sourceFile(directory.filePath(QStringLiteral("Loaded.qml")));
    QVERIFY(sourceFile.open(QIODevice::WriteOnly));
    const QByteArray sourceData = R"(
        import Qtui
        Item { objectName: "urlContent"; implicitWidth: 9 }
    )";
    QCOMPARE(sourceFile.write(sourceData), sourceData.size());
    sourceFile.close();

    QQmlEngine engine;
    addQtuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        Loader { source: "Loaded.qml" }
    )", QUrl::fromLocalFile(directory.filePath(QStringLiteral("Host.qml"))));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *loader = qobject_cast<Loader *>(object.get());
    QVERIFY(loader);
    QCOMPARE(loader->status(), Loader::Ready);
    QVERIFY(loader->item());
    QCOMPARE(loader->item()->objectName(), QStringLiteral("urlContent"));
    QCOMPARE(loader->implicitWidth(), 9);
}

void TestLoader::activeControlsLifetime() {
    QQmlEngine engine;
    addQtuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        Loader {
            active: false
            sourceComponent: Item { implicitWidth: 4; implicitHeight: 2 }
        }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    auto *loader = qobject_cast<Loader *>(object.get());
    QVERIFY(loader);
    QCOMPARE(loader->status(), Loader::Null);
    QVERIFY(!loader->item());

    QSignalSpy loadedSpy(loader, &Loader::loaded);
    loader->setActive(true);
    QCOMPARE(loader->status(), Loader::Ready);
    QVERIFY(loader->item());
    QCOMPARE(loadedSpy.size(), 1);

    QPointer<Item> loadedItem = loader->item();
    loader->setActive(false);
    QCOMPARE(loader->status(), Loader::Null);
    QVERIFY(!loader->item());
    QVERIFY(loadedItem.isNull());
}

void TestLoader::replacingComponentDestroysItem() {
    QQmlEngine engine;
    addQtuiImportPath(engine);

    QQmlComponent first(&engine);
    first.setData(R"(
        import Qtui
        Item { objectName: "first" }
    )", QUrl());
    QVERIFY2(first.isReady(), qPrintable(first.errorString()));

    QQmlComponent second(&engine);
    second.setData(R"(
        import Qtui
        Item { objectName: "second" }
    )", QUrl());
    QVERIFY2(second.isReady(), qPrintable(second.errorString()));

    Loader loader;
    loader.setSourceComponent(&first);
    QCOMPARE(loader.status(), Loader::Ready);
    QPointer<Item> firstItem = loader.item();

    loader.setSourceComponent(&second);
    QVERIFY(firstItem.isNull());
    QCOMPARE(loader.status(), Loader::Ready);
    QVERIFY(loader.item());
    QCOMPARE(loader.item()->objectName(), QStringLiteral("second"));
}

void TestLoader::followsLoadedImplicitSize() {
    QQmlEngine engine;
    addQtuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        Item { implicitWidth: 3; implicitHeight: 2 }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    Loader loader;
    loader.setSourceComponent(&component);
    QCOMPARE(loader.implicitWidth(), 3);
    QCOMPARE(loader.implicitHeight(), 2);

    loader.item()->setImplicitWidth(8);
    loader.item()->setImplicitHeight(5);
    QCOMPARE(loader.implicitWidth(), 8);
    QCOMPARE(loader.implicitHeight(), 5);
}

void TestLoader::propagatesExplicitSize() {
    QQmlEngine engine;
    addQtuiImportPath(engine);

    QQmlComponent component(&engine);
    component.setData(R"(
        import Qtui
        Item { implicitWidth: 3; implicitHeight: 2 }
    )", QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    Loader loader;
    loader.setWidth(20);
    loader.setHeight(7);
    loader.setSourceComponent(&component);

    QVERIFY(loader.item());
    QCOMPARE(loader.item()->width(), 20);
    QCOMPARE(loader.item()->height(), 7);
    QCOMPARE(loader.implicitWidth(), 3);
    QCOMPARE(loader.implicitHeight(), 2);
}

QTEST_MAIN(TestLoader)
#include "test_loader.moc"
