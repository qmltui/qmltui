#include <QTest>
#include <QSignalSpy>
#include <QQmlEngine>
#include <QQmlComponent>
#include <Qamui/Core/Items/Text.h>
#include <Qamui/Core/Primitives/QamuiGlobal.h>
#include "../TestPaint.h"
#include <memory>

using namespace qamui;

static TextGridBuffer frame(Text &text, int width = 30, int height = 5)
{
    TextGridBuffer buffer;
    buffer.resize(width, height);
    testing::paintItem(text, buffer, 0, 0);
    return buffer;
}

class TestSpan : public QObject {
    Q_OBJECT
private slots:
    void values();
    void inheritanceAndUpdates();
    void precedenceAndLines();
    void unicodeAndLiteral();
    void alignmentAndClipping();
    void qmlIntegration();
    void qmlResetAndMutation();
    void factoryValidation();
    void gradient();
};

void TestSpan::values() {
    Span a;
    QVERIFY(!a.color().isValid());
    QVERIFY(!a.backgroundColor().isValid());
    QVERIFY(!a.hasBold());
    Span b = a;
    b.setBold(false);
    QVERIFY(a != b);
    QVERIFY(b.hasBold());
    QCOMPARE(b.resolveAttributes(TextAttribute::Bold), TextAttribute::None);
    b.resetBold();
    QCOMPARE(a, b);
    a = Span(QStringLiteral("x"), Color("default"), Color("#123456"));
    b = a;
    Span moved = std::move(b);
    QCOMPARE(a, moved);
    QVERIFY(a.color().isValid());
    a.resetColor();
    a.resetBackgroundColor();
    QVERIFY(!a.color().isValid());
    QVERIFY(!a.backgroundColor().isValid());
    a.setDim(false); a.setItalic(false); a.setUnderline(false); a.setReversed(false);
    QCOMPARE(a.resolveAttributes(TextAttribute::Dim | TextAttribute::Italic | TextAttribute::Underline | TextAttribute::Reversed), TextAttribute::None);
    a.resetDim(); a.resetItalic(); a.resetUnderline(); a.resetReversed();
    QCOMPARE(a, Span(QStringLiteral("x")));
}

void TestSpan::inheritanceAndUpdates() {
    Text text;
    text.setColor(Color("#ff0000"));
    text.setBold(true);
    Span explicitStyle(QStringLiteral("B"), Color("default"), Color("#123456"));
    explicitStyle.setBold(false);
    explicitStyle.setUnderline(true);
    text.setSpans({Span(QStringLiteral("A")), explicitStyle});
    auto buffer = frame(text);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(1, 0).fgColor, 0u);
    QCOMPARE(buffer.cellAt(1, 0).bgColor, 0xff123456u);
    QCOMPARE(buffer.cellAt(1, 0).attributes, TextAttribute::Underline);
    text.setColor(Color("#0000ff"));
    text.setItalic(true);
    buffer = frame(text);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xff0000ffu);
    QCOMPARE(buffer.cellAt(1, 0).attributes, TextAttribute::Italic | TextAttribute::Underline);
    text.setEnabled(false);
    buffer = frame(text);
    QCOMPARE(buffer.cellAt(1, 0).attributes, TextAttribute::Italic | TextAttribute::Underline | TextAttribute::Dim);
}

void TestSpan::precedenceAndLines() {
    Text text;
    QSignalSpy spy(&text, &Text::spansChanged);
    text.setText(QStringLiteral("fallback"));
    const QList<Span> spans{Span(QStringLiteral("ab")), Span(QStringLiteral("c\n\nx\n"), Color("#123456"))};
    text.setSpans(spans);
    text.setSpans(spans);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(text.implicitWidth(), 3);
    QCOMPARE(text.implicitHeight(), 4);
    auto buffer = frame(text);
    QCOMPARE(buffer.cellAt(2, 0).character(), QByteArrayView("c"));
    QCOMPARE(buffer.cellAt(0, 2).character(), QByteArrayView("x"));
    QCOMPARE(buffer.cellAt(0, 2).fgColor, 0xff123456u);
    text.setText(QStringLiteral("latest"));
    QCOMPARE(text.implicitWidth(), 3);
    text.setSpans({Span()});
    QCOMPARE(text.implicitWidth(), 0);
    QCOMPARE(text.implicitHeight(), 1);
    buffer = frame(text);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView(" "));
    text.setSpans({});
    QCOMPARE(text.implicitWidth(), 6);
    buffer = frame(text);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("l"));
}

void TestSpan::unicodeAndLiteral() {
    Text text;
    text.setSpans({Span(QStringLiteral("e"), Color("#ff0000")),
                   Span(QString::fromUtf8("\xcc\x81中👍"), Color("#0000ff")),
                   Span(QString::fromUtf8("🏽"), Color("#00ff00"))});
    QCOMPARE(text.implicitWidth(), 5);
    auto buffer = frame(text);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("e\xcc\x81"));
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffff0000u);
    QVERIFY(buffer.cellAt(2, 0).continuation);
    QCOMPARE(buffer.cellAt(3, 0).character(), QByteArrayView("👍🏽"));
    QCOMPARE(buffer.cellAt(3, 0).fgColor, 0xff0000ffu);
    text.setSpans({Span(QStringLiteral("<b>x</b>\033[31m"))});
    QCOMPARE(text.implicitWidth(), 12);
    buffer = frame(text);
    QCOMPARE(buffer.cellAt(0, 0).character(), QByteArrayView("<"));
    QCOMPARE(buffer.cellAt(8, 0).character(), QByteArrayView("["));
    QCOMPARE(buffer.cellAt(8, 0).fgColor, 0u);
    text.setTextFormat(Text::RichText);
    QCOMPARE(text.implicitWidth(), 12);
}

void TestSpan::alignmentAndClipping() {
    Text text;
    text.setSpans({Span(QStringLiteral("abc\nx"))});
    text.setWidth(7);
    text.setHeight(5);
    text.setHorizontalAlignment(Qt::AlignRight);
    text.setVerticalAlignment(Qt::AlignBottom);
    auto buffer = frame(text);
    QCOMPARE(buffer.cellAt(4, 3).character(), QByteArrayView("a"));
    QCOMPARE(buffer.cellAt(6, 4).character(), QByteArrayView("x"));
    text.setHorizontalAlignment(Qt::AlignLeft);
    text.setVerticalAlignment(Qt::AlignTop);
    text.setWidth(2);
    buffer = frame(text);
    QCOMPARE(buffer.cellAt(1, 0).character(), QByteArrayView("b"));
    QCOMPARE(buffer.cellAt(2, 0).character(), QByteArrayView(" "));
    text.setWidth(7);
    text.setLeftPadding(1);
    text.setTopPadding(1);
    buffer = frame(text);
    QCOMPARE(buffer.cellAt(1, 1).character(), QByteArrayView("a"));
}

void TestSpan::qmlIntegration() {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import Qamui
        Text {
            bold: true
            spans: [ { text: "A", bold: false, color: "default" },
                     Qamui.span("B", { color: Qamui.rgba(1, 0, 0, 1), backgroundColor: "#123456", dim: true }),
                     Qamui.span("C"), { text: "D", color: "green" } ]
        }
    )", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *text = qobject_cast<Text *>(object.get());
    QVERIFY(text);
    QCOMPARE(text->spans().size(), 4);
    auto buffer = frame(*text);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::None);
    QCOMPARE(buffer.cellAt(1, 0).fgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(1, 0).bgColor, 0xff123456u);
    QCOMPARE(buffer.cellAt(1, 0).attributes, TextAttribute::Bold | TextAttribute::Dim);
    QCOMPARE(buffer.cellAt(2, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(3, 0).fgColor, Color("green").argb32());
}

void TestSpan::qmlResetAndMutation() {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import Qamui
        Text {
            bold: true
            color: "#ff0000"
            spans: [{ text: "A", bold: false, color: "default" }]
            function resetFirst(): void {
                spans[0].bold = undefined
                spans[0].color = undefined
                spans[0].text = "BC"
            }
        }
    )", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *text = qobject_cast<Text *>(object.get());
    QVERIFY(text);
    QSignalSpy spy(text, &Text::spansChanged);
    QVERIFY(QMetaObject::invokeMethod(text, "resetFirst"));
    QCOMPARE(text->implicitWidth(), 2);
    QVERIFY(!text->spans().first().hasBold());
    QVERIFY(!text->spans().first().color().isValid());
    QVERIFY(spy.count() > 0);
    const auto buffer = frame(*text);
    QCOMPARE(buffer.cellAt(0, 0).attributes, TextAttribute::Bold);
    QCOMPARE(buffer.cellAt(0, 0).fgColor, 0xffff0000u);
    QCOMPARE(buffer.cellAt(1, 0).character(), QByteArrayView("C"));
}

void TestSpan::factoryValidation() {
    QamuiGlobal global;
    QTest::ignoreMessage(QtWarningMsg, "Qamui.span: expected boolean for bold");
    QTest::ignoreMessage(QtWarningMsg, "Qamui.span: invalid color for color");
    QTest::ignoreMessage(QtWarningMsg, "Qamui.span: unknown option typo");
    auto span = global.span(QStringLiteral("x"), {{"bold", "false"}, {"color", "bad-color"}, {"typo", true}});
    QVERIFY(!span.hasBold());
    QVERIFY(!span.color().isValid());
}

void TestSpan::gradient() {
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        pragma ComponentBehavior: Bound
        import Qamui
        Text {
            spans: [{ text: "AB", color: "green" }]
            gradient: Gradient {
                GradientStop { position: 0; color: "#ff0000" }
                GradientStop { position: 1; color: "#0000ff" }
            }
        }
    )", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    auto *text = qobject_cast<Text *>(object.get());
    QVERIFY(text);
    QCOMPARE(frame(*text).cellAt(0, 0).fgColor, 0xffff0000u);
}
QTEST_GUILESS_MAIN(TestSpan)
#include "test_span.moc"
