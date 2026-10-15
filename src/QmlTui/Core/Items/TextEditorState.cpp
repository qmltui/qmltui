#include <QmlTui/Core/Items/TextEditorState.h>

#include <QmlTui/Core/Primitives/UnicodeText.h>

#include <algorithm>

namespace qmltui {

int TextEditorState::selectionStart() const {
    return (std::min)(m_cursorPosition, m_selectionAnchor);
}

int TextEditorState::selectionEnd() const {
    return (std::max)(m_cursorPosition, m_selectionAnchor);
}

QString TextEditorState::selectedText() const {
    return m_text.mid(selectionStart(), selectionEnd() - selectionStart());
}

TextEditorState::Snapshot TextEditorState::snapshot() const {
    return Snapshot{m_text, m_cursorPosition, m_selectionAnchor};
}

TextEditorState::Change TextEditorState::changeSince(const Snapshot& before) const {
    return Change{before.text != m_text, before.cursor != m_cursorPosition,
                  before.cursor != m_cursorPosition || before.anchor != m_selectionAnchor};
}

void TextEditorState::restore(const Snapshot& state) {
    m_text = state.text;
    m_cursorPosition = snapToBoundary(m_text, state.cursor);
    m_selectionAnchor = snapToBoundary(m_text, state.anchor);
}

TextEditorState::Change TextEditorState::setText(const QString& text) {
    const Snapshot before = snapshot();
    if (m_text != text) {
        m_text = text;
        m_undoStack.clear();
        m_redoStack.clear();
    }
    m_cursorPosition = snapToBoundary(m_text, m_cursorPosition);
    m_selectionAnchor = snapToBoundary(m_text, m_selectionAnchor);
    return changeSince(before);
}

TextEditorState::Change TextEditorState::moveCursor(int position, bool extendSelection) {
    const Snapshot before = snapshot();
    const int snapped = snapToBoundary(m_text, position);
    m_cursorPosition = snapped;
    if (!extendSelection) m_selectionAnchor = snapped;
    return changeSince(before);
}

TextEditorState::Change TextEditorState::select(int start, int end) {
    const Snapshot before = snapshot();
    m_selectionAnchor = snapToBoundary(m_text, start);
    m_cursorPosition = snapToBoundary(m_text, end);
    return changeSince(before);
}

TextEditorState::Change TextEditorState::selectAll() {
    const Snapshot before = snapshot();
    m_selectionAnchor = 0;
    m_cursorPosition = m_text.size();
    return changeSince(before);
}

TextEditorState::Change TextEditorState::clearSelection() {
    const Snapshot before = snapshot();
    m_selectionAnchor = m_cursorPosition;
    return changeSince(before);
}

void TextEditorState::recordUndo() {
    const Snapshot current = snapshot();
    if (m_undoStack.isEmpty() || !(m_undoStack.last() == current)) m_undoStack.append(current);
    if (m_undoStack.size() > 100) m_undoStack.removeFirst();
    m_redoStack.clear();
}

TextEditorState::Change TextEditorState::replaceRange(int start, int end,
                                                      const QString& replacement) {
    start = snapToBoundary(m_text, start);
    end = snapToBoundary(m_text, end);
    if (start > end) std::swap(start, end);

    QString changed = m_text;
    changed.replace(start, end - start, replacement);
    const int cursor = snapToBoundary(changed, start + replacement.size());
    if (changed == m_text && m_cursorPosition == cursor && m_selectionAnchor == cursor) return {};

    const Snapshot before = snapshot();
    recordUndo();
    m_text = changed;
    m_cursorPosition = cursor;
    m_selectionAnchor = cursor;
    return changeSince(before);
}

TextEditorState::Change TextEditorState::replaceSelection(const QString& replacement) {
    return replaceRange(selectionStart(), selectionEnd(), replacement);
}

TextEditorState::Change TextEditorState::deleteSelection() {
    if (!hasSelection()) return {};
    return replaceRange(selectionStart(), selectionEnd(), {});
}

TextEditorState::Change TextEditorState::deletePreviousCharacter() {
    if (hasSelection()) return deleteSelection();
    if (m_cursorPosition == 0) return {};
    return replaceRange(UnicodeText::previousBoundary(m_text, m_cursorPosition), m_cursorPosition,
                        {});
}

TextEditorState::Change TextEditorState::deleteNextCharacter() {
    if (hasSelection()) return deleteSelection();
    if (m_cursorPosition >= m_text.size()) return {};
    return replaceRange(m_cursorPosition, UnicodeText::nextBoundary(m_text, m_cursorPosition), {});
}

int TextEditorState::previousWordPosition() const {
    int position = UnicodeText::previousBoundary(m_text, m_cursorPosition);
    while (position > 0 && m_text[position - 1].isSpace())
        position = UnicodeText::previousBoundary(m_text, position);
    while (position > 0 && !m_text[position - 1].isSpace())
        position = UnicodeText::previousBoundary(m_text, position);
    return position;
}

int TextEditorState::nextWordPosition() const {
    int position = UnicodeText::nextBoundary(m_text, m_cursorPosition);
    while (position < m_text.size() && !m_text[position].isSpace())
        position = UnicodeText::nextBoundary(m_text, position);
    while (position < m_text.size() && m_text[position].isSpace())
        position = UnicodeText::nextBoundary(m_text, position);
    return position;
}

int TextEditorState::nextWordEndPosition() const {
    int position = m_cursorPosition;
    while (position < m_text.size() && m_text[position].isSpace())
        position = UnicodeText::nextBoundary(m_text, position);
    while (position < m_text.size() && !m_text[position].isSpace())
        position = UnicodeText::nextBoundary(m_text, position);
    return position;
}

TextEditorState::Change TextEditorState::deletePreviousWord() {
    if (hasSelection()) return deleteSelection();
    return replaceRange(previousWordPosition(), m_cursorPosition, {});
}

TextEditorState::Change TextEditorState::deleteNextWord() {
    if (hasSelection()) return deleteSelection();
    return replaceRange(m_cursorPosition, nextWordPosition(), {});
}

TextEditorState::Change TextEditorState::deleteToStart() {
    if (hasSelection()) return deleteSelection();
    return replaceRange(0, m_cursorPosition, {});
}

TextEditorState::Change TextEditorState::deleteToEnd() {
    if (hasSelection()) return deleteSelection();
    return replaceRange(m_cursorPosition, m_text.size(), {});
}

TextEditorState::Change TextEditorState::undo() {
    if (m_undoStack.isEmpty()) return {};
    const Snapshot before = snapshot();
    m_redoStack.append(before);
    restore(m_undoStack.takeLast());
    return changeSince(before);
}

TextEditorState::Change TextEditorState::redo() {
    if (m_redoStack.isEmpty()) return {};
    const Snapshot before = snapshot();
    m_undoStack.append(before);
    restore(m_redoStack.takeLast());
    return changeSince(before);
}

int TextEditorState::snapToBoundary(const QString& text, int position) {
    const int clamped = std::clamp(position, 0, static_cast<int>(text.size()));
    if (clamped == 0 || clamped == text.size()) return clamped;

    const int lineStart = static_cast<int>(
        text.lastIndexOf(u'\n', (std::max)(qsizetype(0), qsizetype(clamped - 1))) + 1);
    GraphemeIterator iterator(QStringView(text).sliced(lineStart));
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        const int start = lineStart + cluster.utf16Start;
        const int end = start + cluster.utf16Length();
        if (clamped == start || clamped == end) return clamped;
        if (clamped > start && clamped < end) return clamped - start <= end - clamped ? start : end;
    }
    return clamped;
}

int TextEditorState::graphemeCount(const QString& text) {
    int count = 0;
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster))
        ++count;
    return count;
}

QString TextEditorState::truncateToGraphemes(const QString& text, int maximumLength) {
    if (maximumLength <= 0) return text;
    int count = 0;
    int keepUntil = 0;
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        ++count;
        if (count == maximumLength) keepUntil = cluster.utf16Start + cluster.utf16Length();
        if (count > maximumLength) return text.left(keepUntil);
    }
    return text;
}

QString TextEditorState::sanitizeSingleLine(const QString& text) {
    QString sanitized;
    sanitized.reserve(text.size());
    for (int index = 0; index < text.size(); ++index) {
        const QChar character = text[index];
        const ushort value = character.unicode();
        if (character == '\r') {
            if (index + 1 < text.size() && text[index + 1] == '\n') ++index;
            sanitized.append(' ');
        } else if (character == '\n' || character == '\t') {
            sanitized.append(' ');
        } else if (value < 0x20 || (value >= 0x7f && value <= 0x9f)) {
            continue;
        } else {
            sanitized.append(character);
        }
    }
    return sanitized;
}

int TextEditorState::columnForPosition(const QString& text, int position) {
    return UnicodeText::displayWidth(text.left(snapToBoundary(text, position)));
}

int TextEditorState::positionForColumn(const QString& text, int column) {
    const int target = (std::max)(0, column);
    int width = 0;
    int position = 0;
    GraphemeIterator iterator(text);
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        if (width + cluster.width > target) break;
        width += cluster.width;
        position = cluster.utf16Start + cluster.utf16Length();
    }
    return position;
}

} // namespace qmltui
