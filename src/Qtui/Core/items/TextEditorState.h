#pragma once

#include <QString>
#include <QVector>

namespace qtui {

class TextEditorState {
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

    static int snapToBoundary(const QString &text, int position);
    static int graphemeCount(const QString &text);
    static QString truncateToGraphemes(const QString &text, int maximumLength);
    static QString sanitizeSingleLine(const QString &text);
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

} // namespace qtui
