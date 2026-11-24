// Compile and runtime verification of the CustomBlock example in docs/guides/custom-items.md.
#include <QTest>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Primitives/Color.h>
#include <QmlTui/Core/Primitives/TextGridBuffer.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>

class CustomBlock : public qmltui::Item {
    Q_OBJECT
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)

public:
    explicit CustomBlock(QObject* parent = nullptr) : qmltui::Item(parent) {
        setFocusPolicy(Qt::StrongFocus);
        setImplicitWidth(16);
        setImplicitHeight(1);
    }

    QString label() const { return m_label; }
    void setLabel(const QString& label) {
        if (m_label == label) return;
        m_label = label;
        setImplicitWidth(qmltui::UnicodeText::displayWidth(m_label) + leftPadding() +
                         rightPadding());
        emit labelChanged();
        markDirty();
    }

    qmltui::Color color() const { return m_color; }
    void setColor(qmltui::Color color) {
        if (m_color == color) return;
        m_color = color;
        emit colorChanged();
        markDirty();
    }

    void paintForTest(qmltui::TextGridBuffer& buffer, int absX, int absY,
                      const qmltui::ClipRect& clipRect) const {
        paint(buffer, absX, absY, clipRect);
    }

    void handleKeyForTest(qmltui::KeyEvent* event) { keyPressEvent(event); }

signals:
    void labelChanged();
    void colorChanged();

protected:
    void paint(qmltui::TextGridBuffer& buffer, int absX, int absY,
               const qmltui::ClipRect& clipRect) const override {
        if (clipRect.isEmpty()) return;
        const int clipRight = clipRect.x + clipRect.width;
        const int clipBottom = clipRect.y + clipRect.height;
        const int row = absY + topPadding();
        if (row < clipRect.y || row >= clipBottom) return;

        int col = absX + leftPadding();
        const qmltui::TextAttribute attrs =
            activeFocus() ? qmltui::TextAttribute::Bold : qmltui::TextAttribute::None;

        qmltui::GraphemeIterator it(m_label);
        qmltui::GraphemeRef g;
        while (it.next(g)) {
            if (g.width > 0 && col + g.width > clipRight) break;
            if (col >= clipRect.x) {
                buffer.setGrapheme(col, row, g.utf16, g.width, m_color, qmltui::Color::Default,
                                   attrs, clipRect.x);
            }
            col += g.width;
        }
    }

    void keyPressEvent(qmltui::KeyEvent* event) override {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            setColor(qmltui::Color::fromString(QStringLiteral("green")));
            event->setAccepted(true);
        }
    }

private:
    QString m_label;
    qmltui::Color m_color = qmltui::Color::fromString(QStringLiteral("cyan"));
};

class TestDocCustomBlock : public QObject {
    Q_OBJECT

private slots:
    void testPaintAndClipAndKey() {
        CustomBlock block;
        block.setLabel(QStringLiteral("Hello"));
        block.setActiveFocus(true);

        qmltui::TextGridBuffer buffer;
        QVERIFY(buffer.resize(10, 2));
        block.paintForTest(buffer, 1, 0, qmltui::ClipRect{2, 0, 3, 1});

        // Column 1 is left of clipRect.x (2); columns 2..4 render "ell" bold.
        QCOMPARE(buffer.cellAt(1, 0).character(), QByteArray(" "));
        QCOMPARE(buffer.cellAt(2, 0).character(), QByteArray("e"));
        QCOMPARE(buffer.cellAt(3, 0).character(), QByteArray("l"));
        QCOMPARE(buffer.cellAt(4, 0).character(), QByteArray("l"));
        QCOMPARE(buffer.cellAt(5, 0).character(), QByteArray(" "));
        QVERIFY((static_cast<uint8_t>(buffer.cellAt(2, 0).attributes) &
                 static_cast<uint8_t>(qmltui::TextAttribute::Bold)) != 0);

        qmltui::KeyEvent ret(Qt::Key_Return, QStringLiteral("\r"), 0);
        block.handleKeyForTest(&ret);
        QVERIFY(ret.isAccepted());
        QCOMPARE(block.color(), qmltui::Color::fromString(QStringLiteral("green")));
    }
};

QTEST_GUILESS_MAIN(TestDocCustomBlock)
#include "test_doc_custom_block.moc"
