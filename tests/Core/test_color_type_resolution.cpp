#include <QMetaProperty>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QStringList>
#include <QTest>
#include <QUrl>
#include <QVariant>

#include <QmlTui/Core/Primitives/Color.h>

#include <memory>

namespace {

QString errorText(const QQmlComponent& component) {
    QStringList messages;
    for (const QQmlError& error : component.errors())
        messages.append(error.toString());
    return messages.join(QLatin1Char('\n'));
}

} // namespace

// Verifies that QML `rgba` properties resolve to `qmltui::Color` via `QmlTui`
// without requiring `QtQuick`, while the Qt Quick built-in `color` value type
// remains unavailable when `QtQuick` is not imported.
class TestColorTypeResolution : public QObject {
    Q_OBJECT

private slots:
    void qmlTuiRgbaPropertyResolvesToColor();
    void builtinColorTypeIsRejectedWithoutQtQuick();
};

void TestColorTypeResolution::qmlTuiRgbaPropertyResolvesToColor() {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"qml(
        import QtQml
        import QmlTui

        QtObject {
            property rgba accent: QmlTui.rgba(0.2, 0.4, 0.6, 1.0)
        }
    )qml",
                      QUrl());

    QVERIFY2(component.isReady(), qPrintable(errorText(component)));

    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);

    const int index = object->metaObject()->indexOfProperty("accent");
    QVERIFY(index >= 0);

    const QMetaProperty property = object->metaObject()->property(index);
    const QVariant value = property.read(object.get());
    const qmltui::Color color = value.value<qmltui::Color>();

    QCOMPARE(QByteArray(property.typeName()), QByteArray("qmltui::Color"));
    QCOMPARE(QByteArray(value.metaType().name()), QByteArray("qmltui::Color"));
    QCOMPARE(color, qmltui::Color("#336699"));
}

void TestColorTypeResolution::builtinColorTypeIsRejectedWithoutQtQuick() {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"qml(
        import QtQml
        import QmlTui

        QtObject {
            property color accent: "#336699"
        }
    )qml",
                      QUrl());

    const QString errors = errorText(component);
    QVERIFY(component.isError());
    QVERIFY2(errors.contains(QStringLiteral("color is not a type")), qPrintable(errors));
}

QTEST_MAIN(TestColorTypeResolution)
#include "test_color_type_resolution.moc"
