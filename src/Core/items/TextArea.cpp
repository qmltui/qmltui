#include "TextArea.h"

#include "../primitives/ColorMapper.h"
#include "../primitives/UnicodeText.h"
#include "../engine/KeyEvent.h"
#include "../engine/MouseEvent.h"
#include "Terminal.h"
#include <algorithm>

namespace qtui {

TextArea::TextArea(QObject* parent) : Item(parent) {
    setFocusable(true);
    setClip(true);
    setImplicitWidth(20);
    setImplicitHeight(5);
}

TextArea::~TextArea() = default;

void TextArea::setText(const QString& text) {
    if (m_text == text) return;
    m_text = text;
    m_cursorPosition = std::min(m_cursorPosition, static_cast<int>(m_text.size()));
    m_selectionAnchor = std::min(m_selectionAnchor, static_cast<int>(m_text.size()));
    int maximumWidth = 0;
    for (const QString &line : m_text.split('\n'))
        maximumWidth = std::max(maximumWidth, UnicodeText::displayWidth(line));
    setImplicitWidth(std::max(1, maximumWidth + leftPadding() + rightPadding()));
    setImplicitHeight(std::max(1, static_cast<int>(m_text.count('\n')) + 1 + topPadding() + bottomPadding()));
    emit textChanged();
    emit cursorPositionChanged();
    emit selectionChanged();
    ensureCursorVisible();
    markDirty();
}

void TextArea::setColor(const QString& color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    emit colorChanged();
    markDirty();
}

void TextArea::setPlaceholderText(const QString &text) {
    if (m_placeholderText == text) return;
    m_placeholderText = text;
    emit placeholderTextChanged();
    markDirty();
}

void TextArea::setCursorPosition(int pos) {
    moveCursor(pos, false);
}

int TextArea::selectionStart() const {
    return std::min(m_cursorPosition, m_selectionAnchor);
}

int TextArea::selectionEnd() const {
    return std::max(m_cursorPosition, m_selectionAnchor);
}

QString TextArea::selectedText() const {
    return m_text.mid(selectionStart(), selectionEnd() - selectionStart());
}

void TextArea::setReadOnly(bool readOnly) {
    if (m_readOnly == readOnly) return;
    m_readOnly = readOnly;
    emit readOnlyChanged();
}

void TextArea::setContentX(int x) {
    const int clamped = std::max(0, x);
    if (m_contentX == clamped) return;
    m_contentX = clamped;
    emit contentXChanged();
    markDirty();
}

void TextArea::setContentY(int y) {
    const int maximum = std::max(0, static_cast<int>(m_text.count('\n')) + 1 - std::max(1, height() - topPadding() - bottomPadding()));
    const int clamped = std::clamp(y, 0, maximum);
    if (m_contentY == clamped) return;
    m_contentY = clamped;
    emit contentYChanged();
    markDirty();
}

void TextArea::selectAll() {
    m_selectionAnchor = 0;
    m_cursorPosition = m_text.size();
    emit cursorPositionChanged();
    emit selectionChanged();
    ensureCursorVisible();
}

void TextArea::clearSelection() {
    if (m_selectionAnchor == m_cursorPosition) return;
    m_selectionAnchor = m_cursorPosition;
    emit selectionChanged();
    markDirty();
}

void TextArea::copy() {
    if (selectedText().isEmpty()) return;
    if (Terminal *term = terminal())
        term->copyToClipboard(selectedText());
}

void TextArea::cut() {
    if (m_readOnly || selectedText().isEmpty()) return;
    copy();
    recordUndo();
    deleteSelection();
}

void TextArea::paste(const QString &text) {
    if (m_readOnly || text.isEmpty()) return;
    recordUndo();
    replaceSelection(text);
}

void TextArea::undo() {
    if (m_readOnly || m_undoStack.isEmpty()) return;
    m_redoStack.append(m_text);
    const QString previous = m_undoStack.takeLast();
    setText(previous);
    moveCursor(std::min(m_cursorPosition, static_cast<int>(previous.size())), false);
}

void TextArea::redo() {
    if (m_readOnly || m_redoStack.isEmpty()) return;
    m_undoStack.append(m_text);
    const QString next = m_redoStack.takeLast();
    setText(next);
    moveCursor(std::min(m_cursorPosition, static_cast<int>(next.size())), false);
}

void TextArea::lineColumnForPosition(int position, int &line, int &column) const {
    const int start = lineStart(position);
    line = m_text.left(start).count('\n');
    column = UnicodeText::displayWidth(m_text.mid(start, position - start));
}

int TextArea::positionForLineColumn(int line, int column) const {
    const QStringList lines = m_text.split('\n');
    if (lines.isEmpty()) return 0;
    line = std::clamp(line, 0, static_cast<int>(lines.size()) - 1);
    int position = 0;
    for (int i = 0; i < line; ++i)
        position += lines[i].size() + 1;
    int width = 0;
    for (const Grapheme &cluster : UnicodeText::graphemes(lines[line])) {
        if (width + cluster.width > column)
            break;
        width += cluster.width;
        position += cluster.utf16Length;
    }
    return position;
}

int TextArea::lineStart(int position) const {
    const int newline = m_text.lastIndexOf('\n', std::max(0, position - 1));
    return newline < 0 ? 0 : newline + 1;
}

int TextArea::lineEnd(int position) const {
    const int newline = m_text.indexOf('\n', position);
    return newline < 0 ? m_text.size() : newline;
}

void TextArea::moveCursor(int position, bool extendSelection) {
    const int clamped = std::clamp(position, 0, static_cast<int>(m_text.size()));
    if (!extendSelection)
        m_selectionAnchor = clamped;
    if (m_cursorPosition == clamped) {
        if (!extendSelection) emit selectionChanged();
        return;
    }
    m_cursorPosition = clamped;
    emit cursorPositionChanged();
    emit selectionChanged();
    ensureCursorVisible();
    markDirty();
}

bool TextArea::deleteSelection() {
    if (selectionStart() == selectionEnd()) return false;
    const int start = selectionStart();
    QString changed = m_text;
    changed.remove(start, selectionEnd() - start);
    m_cursorPosition = start;
    m_selectionAnchor = start;
    m_text.clear();
    setText(changed);
    return true;
}

void TextArea::replaceSelection(const QString &replacement) {
    const int start = selectionStart();
    m_text.replace(start, selectionEnd() - start, replacement);
    m_cursorPosition = start + replacement.size();
    m_selectionAnchor = m_cursorPosition;
    const QString changed = m_text;
    m_text.clear();
    setText(changed);
}

void TextArea::recordUndo() {
    if (m_undoStack.isEmpty() || m_undoStack.last() != m_text)
        m_undoStack.append(m_text);
    if (m_undoStack.size() > 100)
        m_undoStack.removeFirst();
    m_redoStack.clear();
}

void TextArea::ensureCursorVisible() {
    int line = 0;
    int column = 0;
    lineColumnForPosition(m_cursorPosition, line, column);
    const int viewportWidth = std::max(1, width() - leftPadding() - rightPadding());
    const int viewportHeight = std::max(1, height() - topPadding() - bottomPadding());
    if (line < m_contentY) setContentY(line);
    else if (line >= m_contentY + viewportHeight) setContentY(line - viewportHeight + 1);
    if (column < m_contentX) setContentX(column);
    else if (column >= m_contentX + viewportWidth) setContentX(column - viewportWidth + 1);
}

void TextArea::keyPressEvent(KeyEvent* event) {
    if (!activeFocus() || !isEffectivelyEnabled()) return;
    const int key = event->key();
    const bool shift = event->modifiers() & Qt::ShiftModifier;
    const bool control = event->modifiers() & Qt::ControlModifier;

    if (key == Qt::Key_Tab || key == Qt::Key_Backtab)
        return;
    if (control && key == Qt::Key_A) {
        selectAll();
    } else if (control && key == Qt::Key_C) {
        if (selectedText().isEmpty())
            return;
        copy();
    } else if (control && key == Qt::Key_X) {
        cut();
    } else if (control && key == Qt::Key_Z) {
        undo();
    } else if (control && key == Qt::Key_Y) {
        redo();
    } else if (key == Qt::Key_Left) {
        int position = UnicodeText::previousBoundary(m_text, m_cursorPosition);
        if (control) {
            while (position > 0 && m_text[position - 1].isSpace())
                position = UnicodeText::previousBoundary(m_text, position);
            while (position > 0 && !m_text[position - 1].isSpace())
                position = UnicodeText::previousBoundary(m_text, position);
        }
        moveCursor(position, shift);
    } else if (key == Qt::Key_Right) {
        int position = UnicodeText::nextBoundary(m_text, m_cursorPosition);
        if (control) {
            while (position < m_text.size() && !m_text[position].isSpace())
                position = UnicodeText::nextBoundary(m_text, position);
            while (position < m_text.size() && m_text[position].isSpace())
                position = UnicodeText::nextBoundary(m_text, position);
        }
        moveCursor(position, shift);
    } else if (key == Qt::Key_Home) {
        moveCursor(lineStart(m_cursorPosition), shift);
    } else if (key == Qt::Key_End) {
        moveCursor(lineEnd(m_cursorPosition), shift);
    } else if (key == Qt::Key_Up || key == Qt::Key_Down) {
        int line = 0, column = 0;
        lineColumnForPosition(m_cursorPosition, line, column);
        moveCursor(positionForLineColumn(line + (key == Qt::Key_Up ? -1 : 1), column), shift);
    } else if (key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
        int line = 0, column = 0;
        lineColumnForPosition(m_cursorPosition, line, column);
        const int page = std::max(1, height() - topPadding() - bottomPadding());
        moveCursor(positionForLineColumn(
            line + (key == Qt::Key_PageUp ? -page : page), column), shift);
    } else if (!m_readOnly && key == Qt::Key_Backspace) {
        recordUndo();
        if (!deleteSelection() && m_cursorPosition > 0) {
            const int previous = UnicodeText::previousBoundary(m_text, m_cursorPosition);
            m_text.remove(previous, m_cursorPosition - previous);
            m_cursorPosition = previous;
            m_selectionAnchor = previous;
            const QString changed = m_text;
            m_text.clear();
            setText(changed);
        }
    } else if (!m_readOnly && key == Qt::Key_Delete) {
        recordUndo();
        if (!deleteSelection() && m_cursorPosition < m_text.size()) {
            const int next = UnicodeText::nextBoundary(m_text, m_cursorPosition);
            m_text.remove(m_cursorPosition, next - m_cursorPosition);
            const QString changed = m_text;
            m_text.clear();
            setText(changed);
        }
    } else if (!m_readOnly && (key == Qt::Key_Return || key == Qt::Key_Enter)) {
        recordUndo();
        replaceSelection("\n");
    } else if (!m_readOnly && !event->text().isEmpty()) {
        recordUndo();
        replaceSelection(event->text());
    } else {
        return;
    }
    event->setAccepted(true);
}

void TextArea::mousePressEvent(MouseEvent* event) {
    setFocus(true);
    moveCursor(positionAtMouse(event->x(), event->y()), false);
    event->setAccepted(true);
}

void TextArea::mouseMoveEvent(MouseEvent *event) {
    moveCursor(positionAtMouse(event->x(), event->y()), true);
    event->setAccepted(true);
}

int TextArea::positionAtMouse(int x, int y) const {
    const int line = m_contentY + std::max(0, y - topPadding());
    const int column = m_contentX + std::max(0, x - leftPadding());
    return positionForLineColumn(line, column);
}

void TextArea::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    Item::render(buffer, absX, absY, clipRect);
    const int viewportWidth = std::max(0, width() - leftPadding() - rightPadding());
    const int viewportHeight = std::max(0, height() - topPadding() - bottomPadding());
    const ClipRect contentClip = clipRect.intersected(
        ClipRect{absX + leftPadding(), absY + topPadding(), viewportWidth, viewportHeight});

    const QString displayText = m_text.isEmpty() && !activeFocus() ? m_placeholderText : m_text;
    const uint32_t displayColor = !isEffectivelyEnabled() ? 0xFF808080
        : (m_text.isEmpty() && !activeFocus() ? 0xFF808080 : m_resolvedColor);
    const QStringList lines = displayText.split('\n');
    int globalOffset = 0;
    for (int line = 0; line < m_contentY && line < lines.size(); ++line)
        globalOffset += lines[line].size() + 1;

    for (int visibleLine = 0; visibleLine < viewportHeight; ++visibleLine) {
        const int lineIndex = m_contentY + visibleLine;
        if (lineIndex >= lines.size()) break;
        const int y = absY + topPadding() + visibleLine;
        int displayColumn = 0;
        for (const Grapheme &cluster : UnicodeText::graphemes(lines[lineIndex])) {
            const int x = absX + leftPadding() + displayColumn - m_contentX;
            if (x >= absX + leftPadding() && x + cluster.width <= absX + leftPadding() + viewportWidth &&
                contentClip.contains(x, y)) {
                buffer.writeText(x, y, QString::fromUtf8(cluster.utf8), displayColor);
                const int clusterStart = globalOffset + cluster.utf16Start;
                const int clusterEnd = clusterStart + cluster.utf16Length;
                if (!m_text.isEmpty() && clusterEnd > selectionStart() && clusterStart < selectionEnd()) {
                    for (int dx = 0; dx < std::max(1, cluster.width); ++dx)
                        buffer.cellAt(x + dx, y).attributes =
                            buffer.cellAt(x + dx, y).attributes | TextAttribute::Reversed;
                }
            }
            displayColumn += cluster.width;
        }
        globalOffset += lines[lineIndex].size() + 1;
    }

    if (activeFocus()) {
        int line = 0, column = 0;
        lineColumnForPosition(m_cursorPosition, line, column);
        const int x = absX + leftPadding() + column - m_contentX;
        const int y = absY + topPadding() + line - m_contentY;
        if (contentClip.contains(x, y)) {
            Cell &cell = buffer.cellAt(x, y);
            if (cell.character.isEmpty()) cell.character = " ";
            cell.attributes = cell.attributes | TextAttribute::Reversed;
        }
    }
}

} // namespace qtui
