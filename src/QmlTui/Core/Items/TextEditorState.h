#pragma once

#include <QmlTui/Core/Primitives/QmlTuiExport.h>
#include <QString>
#include <QVector>

namespace qmltui {

// Grapheme-boundary-aware text buffer, cursor/selection model, and undo/redo
// stack shared by TextInput and TextArea. All positions are UTF-16 code-unit
// offsets snapped to extended grapheme cluster boundaries.
class QMLTUI_CORE_EXPORT TextEditorState {
public:
    struct Snapshot {
        QString text;
        int cursor = 0;
        int anchor = 0;

        bool operator==(const Snapshot &) const = default;
    };

    struct Change {
        bool text = false;
        bool cursor = false;
        bool selection = false;

        bool any() const { return text || cursor || selection; }
    };

    const QString &text() const { return m_text; }
    int cursorPosition() const { return m_cursorPosition; }
    int selectionAnchor() const { return m_selectionAnchor; }
    int selectionStart() const;
    int selectionEnd() const;
    QString selectedText() const;
    bool hasSelection() const { return m_cursorPosition != m_selectionAnchor; }
    bool canUndo() const { return !m_undoStack.isEmpty(); }
    bool canRedo() const { return !m_redoStack.isEmpty(); }

    Change setText(const QString &text);
    Change moveCursor(int position, bool extendSelection = false);
    Change select(int start, int end);
    Change selectAll();
    Change clearSelection();
    Change replaceSelection(const QString &replacement);
    Change deleteSelection();
    Change deletePreviousCharacter();
    Change deleteNextCharacter();
    Change deletePreviousWord();
    Change deleteNextWord();
    Change deleteToStart();
    Change deleteToEnd();
    Change undo();
    Change redo();

    int previousWordPosition() const;
    int nextWordPosition() const;
    int nextWordEndPosition() const;

    // Clamps `position` into `[0, text.size()]` and snaps it to the nearest
    // valid grapheme boundary at or before `position`.
    static int snapToBoundary(const QString &text, int position);
    static int graphemeCount(const QString &text);
    static QString truncateToGraphemes(const QString &text, int maximumLength);
    static QString sanitizeSingleLine(const QString &text);
    // Converts between UTF-16 code-unit offsets and terminal display columns.
    static int columnForPosition(const QString &text, int position);
    static int positionForColumn(const QString &text, int column);

private:
    Snapshot snapshot() const;
    Change changeSince(const Snapshot &before) const;
    void recordUndo();
    Change replaceRange(int start, int end, const QString &replacement);
    void restore(const Snapshot &snapshot);

    QString m_text;
    int m_cursorPosition = 0;
    int m_selectionAnchor = 0;
    QVector<Snapshot> m_undoStack;
    QVector<Snapshot> m_redoStack;
};

} // namespace qmltui
