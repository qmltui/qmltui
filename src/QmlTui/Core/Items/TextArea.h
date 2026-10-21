#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/TextEditorState.h>
#include <QString>
#include <QmlTui/Core/Primitives/HighlightRange.h>
#include <QMap>
#include <vector>

namespace qmltui {

class ClockTimer;

// Multi-line editable text item with 2D scrolling (`contentX`/`contentY`),
// selection, undo/redo, and revision-checked syntax/diagnostic highlight layers.
class QMLTUI_CORE_EXPORT TextArea : public Item {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(qmltui::Color color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY
                   placeholderTextChanged)
    Q_PROPERTY(
        int cursorPosition READ cursorPosition WRITE setCursorPosition NOTIFY cursorPositionChanged)
    Q_PROPERTY(int selectionStart READ selectionStart NOTIFY selectionChanged)
    Q_PROPERTY(int selectionEnd READ selectionEnd NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
    Q_PROPERTY(bool readOnly READ readOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(int contentX READ contentX WRITE setContentX NOTIFY contentXChanged)
    Q_PROPERTY(int contentY READ contentY WRITE setContentY NOTIFY contentYChanged)
    Q_PROPERTY(
        CursorShape cursorShape READ cursorShape WRITE setCursorShape NOTIFY cursorShapeChanged)
    Q_PROPERTY(
        qmltui::Color cursorColor READ cursorColor WRITE setCursorColor NOTIFY cursorColorChanged)
    Q_PROPERTY(bool cursorBlinking READ cursorBlinking WRITE setCursorBlinking NOTIFY
                   cursorBlinkingChanged)
    Q_PROPERTY(int cursorBlinkRate READ cursorBlinkRate WRITE setCursorBlinkRate NOTIFY
                   cursorBlinkRateChanged)
    Q_PROPERTY(
        bool cursorVisible READ cursorVisible WRITE setCursorVisible NOTIFY cursorVisibleChanged)
    Q_PROPERTY(qmltui::Color selectionColor READ selectionColor WRITE setSelectionColor NOTIFY
                   selectionColorChanged)
    Q_PROPERTY(qmltui::Color selectedTextColor READ selectedTextColor WRITE setSelectedTextColor
                   NOTIFY selectedTextColorChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY canUndoChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY canRedoChanged)
    // Monotonically incremented on every text mutation; pass to `setHighlightLayer`
    // so stale async highlight results are rejected.
    Q_PROPERTY(quint64 textRevision READ textRevision NOTIFY textRevisionChanged)
    Q_PROPERTY(bool tabChangesFocus READ tabChangesFocus WRITE setTabChangesFocus NOTIFY
                   tabChangesFocusChanged)
    Q_PROPERTY(int tabStopDistance READ tabStopDistance WRITE setTabStopDistance NOTIFY
                   tabStopDistanceChanged)
    Q_PROPERTY(bool showWhitespace READ showWhitespace WRITE setShowWhitespace NOTIFY
                   showWhitespaceChanged)

public:
    enum CursorShape : uint8_t {
        Block,
        Underline,
        Beam,
        HollowBlock,
    };
    Q_ENUM(CursorShape)

    explicit TextArea(QObject* parent = nullptr);
    ~TextArea() override;

    QString text() const { return m_editor.text(); }
    void setText(const QString& text);
    Color color() const { return m_color; }
    void setColor(Color color);
    QString placeholderText() const { return m_placeholderText; }
    void setPlaceholderText(const QString& text);
    int cursorPosition() const { return m_editor.cursorPosition(); }
    void setCursorPosition(int pos);
    int selectionStart() const;
    int selectionEnd() const;
    QString selectedText() const;
    bool readOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);
    int contentX() const { return m_contentX; }
    void setContentX(int x);
    int contentY() const { return m_contentY; }
    void setContentY(int y);

    CursorShape cursorShape() const { return m_cursorShape; }
    void setCursorShape(CursorShape shape);
    Color cursorColor() const { return m_cursorColor; }
    void setCursorColor(Color color);
    bool cursorBlinking() const { return m_cursorBlinking; }
    void setCursorBlinking(bool blinking);
    int cursorBlinkRate() const { return m_cursorBlinkRate; }
    void setCursorBlinkRate(int rate);
    bool cursorVisible() const { return m_cursorVisible; }
    void setCursorVisible(bool visible);
    Color selectionColor() const { return m_selectionColor; }
    void setSelectionColor(Color color);
    Color selectedTextColor() const { return m_selectedTextColor; }
    void setSelectedTextColor(Color color);
    bool canUndo() const { return m_editor.canUndo(); }
    bool canRedo() const { return m_editor.canRedo(); }

    quint64 textRevision() const { return m_textRevision; }
    bool tabChangesFocus() const { return m_tabChangesFocus; }
    void setTabChangesFocus(bool tabChangesFocus);
    int tabStopDistance() const { return m_tabStopDistance; }
    void setTabStopDistance(int distance);
    bool showWhitespace() const { return m_showWhitespace; }
    void setShowWhitespace(bool show);

    // Replaces the named highlight layer if `revision == textRevision()`;
    // higher `priority` layers win when styles overlap. Returns false on revision mismatch.
    Q_INVOKABLE bool setHighlightLayer(const QString& name, const QList<HighlightRange>& ranges,
                                       quint64 revision, int priority = 0);
    Q_INVOKABLE void clearHighlightLayer(const QString& name);
    Q_INVOKABLE void clearHighlights();

    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void copy();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void paste(const QString& text);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    void keyPressEvent(class KeyEvent* event) override;
    void mousePressEvent(class MouseEvent* event) override;
    void mouseMoveEvent(class MouseEvent* event) override;
    void wheelEvent(class MouseEvent* event) override;
    void paint(TextGridBuffer& buffer, int absX, int absY,
               const ClipRect& clipRect = ClipRect()) const override;

signals:
    void textChanged();
    void colorChanged();
    void placeholderTextChanged();
    void cursorPositionChanged();
    void selectionChanged();
    void readOnlyChanged();
    void contentXChanged();
    void contentYChanged();
    void cursorShapeChanged();
    void cursorColorChanged();
    void cursorBlinkingChanged();
    void cursorBlinkRateChanged();
    void cursorVisibleChanged();
    void selectionColorChanged();
    void selectedTextColorChanged();
    void canUndoChanged();
    void canRedoChanged();
    void textRevisionChanged();
    void tabChangesFocusChanged();
    void tabStopDistanceChanged();
    void showWhitespaceChanged();
    void highlightsChanged();
    void accepted();

private:
    struct HighlightLayer {
        int priority = 0;
        QList<HighlightRange> ranges;
        bool operator==(const HighlightLayer& other) const noexcept {
            return priority == other.priority && ranges == other.ranges;
        }
        bool operator!=(const HighlightLayer& other) const noexcept { return !(*this == other); }
    };

    struct ComposedRun {
        int start = 0;
        int end = 0;
        TextStyle style;
        bool operator==(const ComposedRun& other) const noexcept {
            return start == other.start && end == other.end && style == other.style;
        }
        bool operator!=(const ComposedRun& other) const noexcept { return !(*this == other); }
    };

    void applyEditorChange(const TextEditorState::Change& change);
    void lineColumnForPosition(int position, int& line, int& column) const;
    int positionForLineColumn(int line, int column) const;
    void moveCursor(int position, bool extendSelection);
    void ensureCursorVisible();
    int lineStart(int position) const;
    int lineEnd(int position) const;
    int positionAtMouse(int x, int y) const;
    void resetBlinkTimer();
    void updateTerminalConnection();

    void ensureGraphemeBoundaries();
    void snapRangeOutward(int start, int length, int& outStart, int& outEnd);
    void recomposeHighlightRuns();

    TextEditorState m_editor;
    Color m_color = "default";
    QString m_placeholderText;
    uint32_t m_resolvedColor = Color::resolve(m_color, 0xFFFFFFFF);
    Color m_cursorColor;
    uint32_t m_resolvedCursorColor = 0;
    bool m_hasResolvedCursorColor = false;
    CursorShape m_cursorShape = Block;
    bool m_cursorBlinking = false;
    int m_cursorBlinkRate = 500;
    bool m_cursorVisible = true;
    ClockTimer* m_blinkTimer = nullptr;
    bool m_blinkState = true;
    Color m_selectionColor;
    uint32_t m_resolvedSelectionColor = 0;
    bool m_hasResolvedSelectionColor = false;
    Color m_selectedTextColor;
    uint32_t m_resolvedSelectedTextColor = 0;
    bool m_hasResolvedSelectedTextColor = false;
    bool m_lastCanUndo = false;
    bool m_lastCanRedo = false;
    int m_contentX = 0;
    int m_contentY = 0;
    bool m_readOnly = false;
    bool m_tabChangesFocus = false;
    int m_tabStopDistance = 4;
    bool m_showWhitespace = false;

    quint64 m_textRevision = 0;
    QMap<QString, HighlightLayer> m_highlightLayers;
    std::vector<ComposedRun> m_composedRuns;
    std::vector<int> m_graphemeBoundaries;
    bool m_graphemeBoundariesValid = false;
    QPointer<Terminal> m_observedTerminal;
    QMetaObject::Connection m_appActiveConnection;
    qint64 m_lastClickMs = -1;
    int m_lastClickPos = -1;
};

} // namespace qmltui
