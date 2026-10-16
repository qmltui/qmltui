// Text caches its parsed, segmented layout between frames. These tests pin
// that the cache is invalidated by every property that affects it and that
// cached paints are identical to a freshly configured item.
#include <QCoreApplication>
#include <QObject>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QTest>
#include <functional>
#include <memory>
#include <QmlTui/Core/Primitives/Color.h>

#include <QSignalSpy>
#include <QmlTui/Core/Engine/MouseEvent.h>

#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include "../TestPaint.h"

using namespace qmltui;
using qmltui::testing::paintItem;

namespace {

TextGridBuffer paintToBuffer(Text &text, int width = 40, int height = 3)
{
    TextGridBuffer buffer;
    buffer.resize(width, height);
    paintItem(text, buffer, 0, 0);
    return buffer;
}

bool buffersEqual(const TextGridBuffer &a, const TextGridBuffer &b)
{
    if (a.width() != b.width() || a.height() != b.height())
        return false;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x)
            if (!(a.cellAt(x, y) == b.cellAt(x, y)))
                return false;
    return true;
}

QString rowText(const TextGridBuffer &buffer, int y)
{
    QString text;
    for (int x = 0; x < buffer.width(); ++x)
        if (!buffer.cellAt(x, y).continuation)
            text += QString::fromUtf8(buffer.cellAt(x, y).character());
    return text;
}

} // namespace

class TestTextLayout : public QObject {
    Q_OBJECT

private slots:
    void testRepaintMatchesFreshItem();
    void testStyleChangeInvalidatesCache();
    void testTextChangeUpdatesImplicitSize();
    void testAnsiAndHtmlMarkup();
    void testEscapeAdvanceQuirk();
    void testAlignmentAndClippingUseCachedWidth();
    void testGradientPerColumn();
    void testWordWrap();
    void testLinkActivated();
};

void TestTextLayout::testRepaintMatchesFreshItem()
{
    const QString markup = QStringLiteral("Mixed <b>bold</b> \033[32mgreen\033[0m ") + QString::fromUtf8("中é👍🏽");
    using Step = std::function<void(Text &)>;
    const QList<Step> steps = {
        [&](Text &t) { t.setText(markup); },
        [](Text &t) { t.setBold(true); },
        [](Text &t) { t.setItalic(true); },
        [](Text &t) { t.setUnderline(true); },
        [](Text &t) { t.setReversed(true); },
        [](Text &t) { t.setColor(Color("#123456")); },
        [](Text &t) { t.setTextFormat(Text::RichText); },
        [](Text &t) { t.setTextFormat(Text::AnsiText); },
        [](Text &t) { t.setTextFormat(Text::PlainText); },
        [](Text &t) { t.setHorizontalAlignment(Qt::AlignRight); },
        [](Text &t) { t.setWidth(30); },
        [](Text &t) { t.setText(QStringLiteral("changed\nsecond line")); },
        [](Text &t) { t.setBold(false); },
    };

    Text cached;
    for (int applied = 0; applied < steps.size(); ++applied) {
        steps[applied](cached);
        const TextGridBuffer cachedFrame = paintToBuffer(cached);

        Text fresh;
        for (int i = 0; i <= applied; ++i)
            steps[i](fresh);
        const TextGridBuffer freshFrame = paintToBuffer(fresh);
        QVERIFY2(buffersEqual(cachedFrame, freshFrame),
                 qPrintable(QStringLiteral("step %1: '%2' vs '%3'").arg(applied)
                            .arg(rowText(cachedFrame, 0), rowText(freshFrame, 0))));
        QCOMPARE(cached.implicitWidth(), fresh.implicitWidth());
        QCOMPARE(cached.implicitHeight(), fresh.implicitHeight());
    }
}

void TestTextLayout::testStyleChangeInvalidatesCache()
{
    Text text;
    text.setText(QStringLiteral("x"));
    TextGridBuffer first = paintToBuffer(text, 2, 1);
    QCOMPARE(first.cellAt(0, 0).attributes, TextAttribute::None);

    text.setBold(true);
    TextGridBuffer bold = paintToBuffer(text, 2, 1);
    QCOMPARE(bold.cellAt(0, 0).attributes, TextAttribute::Bold);

    text.setColor(Color("#ff0000"));
    TextGridBuffer red = paintToBuffer(text, 2, 1);
    QCOMPARE(red.cellAt(0, 0).fgColor, 0xFFFF0000u);
    QCOMPARE(red.cellAt(0, 0).attributes, TextAttribute::Bold);

    // Repainting without changes yields the same cells.
    TextGridBuffer again = paintToBuffer(text, 2, 1);
    QVERIFY(buffersEqual(red, again));
}

void TestTextLayout::testTextChangeUpdatesImplicitSize()
{
    Text text;
    text.setText(QStringLiteral("ab\n") + QString::fromUtf8("中文字") + QStringLiteral("\n<b>x</b>"));
    QCOMPARE(text.implicitHeight(), 3);
    QCOMPARE(text.implicitWidth(), 6);   // 3 wide characters
    text.setTextFormat(Text::PlainText);
    QCOMPARE(text.implicitWidth(), 8);   // "<b>x</b>" is now literal
    text.setText(QString());
    QCOMPARE(text.implicitHeight(), 1);
    QCOMPARE(text.implicitWidth(), 0);
}

void TestTextLayout::testAnsiAndHtmlMarkup()
{
    Text text;
    text.setColor(Color("#00ff00"));
    text.setText(QStringLiteral("\033[1;31mRe\033[0md <font color='blue'>b</font>\033[48;2;1;2;3mZ"));
    TextGridBuffer frame = paintToBuffer(text, 12, 1);
    QCOMPARE(rowText(frame, 0).trimmed(), QStringLiteral("Red bZ"));
    QCOMPARE(frame.cellAt(0, 0).fgColor, Color::fromAnsiIndex(1).argb32());   // SGR 31 is palette red
    QCOMPARE(frame.cellAt(0, 0).attributes, TextAttribute::Bold);
    QCOMPARE(frame.cellAt(2, 0).fgColor, 0xFF00FF00u);       // SGR 0 restores the item colour
    QCOMPARE(frame.cellAt(2, 0).attributes, TextAttribute::None);
    QCOMPARE(frame.cellAt(4, 0).fgColor, Color("blue").argb32());   // <font color> by name
    QCOMPARE(frame.cellAt(5, 0).bgColor, 0xFF010203u);       // 48;2;r;g;b
}

void TestTextLayout::testEscapeAdvanceQuirk()
{
    // A raw control character has zero advance, so the following cluster
    // overwrites its visible replacement; width and cells match 0.3.
    Text text;
    text.setTextFormat(Text::PlainText);
    text.setText(QStringLiteral("a\x1b" "b"));
    QCOMPARE(text.implicitWidth(), 2);
    TextGridBuffer frame = paintToBuffer(text, 4, 1);
    QCOMPARE(rowText(frame, 0), QStringLiteral("ab  "));
}

void TestTextLayout::testAlignmentAndClippingUseCachedWidth()
{
    Text text;
    text.setText(QStringLiteral("abc"));
    text.setWidth(7);
    text.setHorizontalAlignment(Qt::AlignRight);
    TextGridBuffer right = paintToBuffer(text, 7, 1);
    QCOMPARE(rowText(right, 0), QStringLiteral("    abc"));

    text.setHorizontalAlignment(Qt::AlignHCenter);
    TextGridBuffer centre = paintToBuffer(text, 7, 1);
    QCOMPARE(rowText(centre, 0), QStringLiteral("  abc  "));

    text.setWidth(2);
    text.setHorizontalAlignment(Qt::AlignLeft);
    TextGridBuffer clipped = paintToBuffer(text, 7, 1);
    QCOMPARE(rowText(clipped, 0), QStringLiteral("ab     "));
}

void TestTextLayout::testGradientPerColumn()
{
    QQmlEngine engine;
    engine.addImportPath(QCoreApplication::applicationDirPath() + QStringLiteral("/.."));
    QQmlComponent component(&engine);
    component.setData(R"(
        import QmlTui
        Text {
            text: "ABCD"
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#ff0000" }
                GradientStop { position: 1.0; color: "#0000ff" }
            }
        }
    )", QUrl());
    std::unique_ptr<QObject> object(component.create());
    QVERIFY2(object != nullptr, qPrintable(component.errorString()));
    auto *text = qobject_cast<Text *>(object.get());
    QVERIFY(text);

    TextGridBuffer first = paintToBuffer(*text, 6, 1);
    QCOMPARE(first.cellAt(0, 0).fgColor, 0xFFFF0000u);
    QVERIFY(first.cellAt(1, 0).fgColor != first.cellAt(0, 0).fgColor);
    QVERIFY(first.cellAt(2, 0).fgColor != first.cellAt(1, 0).fgColor);
    QVERIFY(((first.cellAt(1, 0).fgColor >> 16) & 0xFF) >
            ((first.cellAt(2, 0).fgColor >> 16) & 0xFF));
    QVERIFY((first.cellAt(1, 0).fgColor & 0xFF) < (first.cellAt(2, 0).fgColor & 0xFF));
    QCOMPARE(first.cellAt(3, 0).fgColor, 0xFF0000FFu);

    TextGridBuffer second = paintToBuffer(*text, 6, 1);
    QVERIFY(buffersEqual(first, second));
}

void TestTextLayout::testWordWrap()
{
    Text text;
    text.setWrapMode(Text::WordWrap);
    text.setWidth(10);
    text.setText(QStringLiteral("Hello world this is wrapped"));

    TextGridBuffer buffer = paintToBuffer(text, 10, 5);
    QCOMPARE(rowText(buffer, 0).trimmed(), QStringLiteral("Hello"));
    QCOMPARE(rowText(buffer, 1).trimmed(), QStringLiteral("world this"));
    QCOMPARE(rowText(buffer, 2).trimmed(), QStringLiteral("is wrapped"));
    QCOMPARE(text.implicitHeight(), 3);

    // Test WrapAnywhere
    text.setWrapMode(Text::WrapAnywhere);
    text.setText(QStringLiteral("ABCDEFGHIJ12345"));
    TextGridBuffer anyBuffer = paintToBuffer(text, 10, 3);
    QCOMPARE(rowText(anyBuffer, 0).trimmed(), QStringLiteral("ABCDEFGHIJ"));
    QCOMPARE(rowText(anyBuffer, 1).trimmed(), QStringLiteral("12345"));
    QCOMPARE(text.implicitHeight(), 2);
}

void TestTextLayout::testLinkActivated()
{
    // HTML link
    {
        Text text;
        text.setWidth(30);
        text.setHeight(3);
        text.setTextFormat(Text::RichText);
        text.setText(QStringLiteral("Click <a href=\"https://example.com\">here</a> for info"));

        QSignalSpy spy(&text, &Text::linkActivated);
        MouseEvent event(MouseEvent::Type::Press, 7, 0, Qt::LeftButton, Qt::NoModifier);
        text.mousePressEvent(&event);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("https://example.com"));
        QVERIFY(event.isAccepted());
    }

    // Markdown link
    {
        Text text;
        text.setWidth(30);
        text.setHeight(3);
        text.setTextFormat(Text::AutoText);
        text.setText(QStringLiteral("Click [here](https://qmltui.org) for info"));

        QSignalSpy spy(&text, &Text::linkActivated);
        MouseEvent event(MouseEvent::Type::Press, 7, 0, Qt::LeftButton, Qt::NoModifier);
        text.mousePressEvent(&event);

        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("https://qmltui.org"));
        QVERIFY(event.isAccepted());
    }
}

QTEST_MAIN(TestTextLayout)
#include "test_text_layout.moc"
