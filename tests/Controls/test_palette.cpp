// The Palette singleton: one shared instance, Qt-named roles, presets, and
// controls that follow it through bindings.
#include <QQmlComponent>
#include <QQmlEngine>
#include <QSignalSpy>
#include <QTest>
#include <memory>

#include "Color.h"
#include "Terminal.h"

using namespace qtui;

class TestPalette : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { qputenv("QTUI_INSPECT", "1"); }
    void defaultPresetIsTerminalColors();
    void presetSwitchUpdatesControls();

private:
    std::unique_ptr<QObject> load(const char* qml)
    {
        QQmlComponent component(&m_engine);
        component.setData(qml, QUrl());
        if (!component.isReady())
            qWarning().noquote() << component.errorString();
        return std::unique_ptr<QObject>(component.create());
    }
    QQmlEngine m_engine;
};

void TestPalette::defaultPresetIsTerminalColors()
{
    auto root = load("import QtQml\nimport Qtui\nimport Qtui.Controls\n"
                     "Terminal {\n"
                     "    property rgba highlight: Palette.highlight\n"
                     "    property rgba window: Palette.window\n"
                     "    function reset() { Palette.applyPreset(Palette.Terminal) }\n"
                     "}");
    QVERIFY(root);
    QMetaObject::invokeMethod(root.get(), "reset");
    QCOMPARE(root->property("highlight").value<Color>(), Color::fromString("yellow"));
    QCOMPARE(root->property("window").value<Color>(), Color::fromString("transparent"));
}

void TestPalette::presetSwitchUpdatesControls()
{
    auto root = load("import QtQml\nimport Qtui\nimport Qtui.Controls\n"
                     "Terminal {\n"
                     "    property alias button: theButton\n"
                     "    property int changes: 0\n"
                     "    Connections { target: Palette; function onChanged() { changes++ } }\n"
                     "    function dark() { Palette.applyPreset(Palette.Dark) }\n"
                     "    function reset() { Palette.applyPreset(Palette.Terminal) }\n"
                     "    Button { id: theButton; text: \"OK\"; focus: true }\n"
                     "}");
    QVERIFY(root);
    QMetaObject::invokeMethod(root.get(), "reset");
    auto* button = qvariant_cast<QObject*>(root->property("button"));
    QVERIFY(button);
    QTRY_VERIFY(button->property("activeFocus").toBool());
    auto* border = qvariant_cast<QObject*>(button->property("border"));
    QVERIFY(border);
    QCOMPARE(border->property("color").value<Color>(), Color::fromString("yellow"));

    const int before = root->property("changes").toInt();
    QMetaObject::invokeMethod(root.get(), "dark");
    QCOMPARE(root->property("changes").toInt(), before + 1);   // one signal for the whole preset
    QCOMPARE(border->property("color").value<Color>(), Color::fromString("#2cde85"));
    QCOMPARE(button->property("color").value<Color>(), Color::fromString("#21262d"));

    QMetaObject::invokeMethod(root.get(), "reset");
    QCOMPARE(border->property("color").value<Color>(), Color::fromString("yellow"));
}

QTEST_MAIN(TestPalette)
#include "test_palette.moc"
