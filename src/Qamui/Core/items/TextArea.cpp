#include <Qamui/Core/items/TextArea.h>

#include <Qamui/Core/engine/KeyEvent.h>
#include <Qamui/Core/engine/MouseEvent.h>
#include <Qamui/Core/primitives/ColorMapper.h>
#include <Qamui/Core/primitives/UnicodeText.h>
#include <Qamui/Core/items/Terminal.h>

#include <algorithm>

namespace qamui {

TextArea::TextArea(QObject* parent) : Item(parent) {
    setFocusable(true);
    setClip(true);
    setImplicitWidth(20);
    setImplicitHeight(5);
}

TextArea::~TextArea() = default;

void TextArea::applyEditorChange(const TextEditorState::Change &change) {
    if (change.text) {
        int maximumWidth = 0;
        for (const QString &line : m_editor.text().split('\n'))
            maximumWidth = std::max(maximumWidth, UnicodeText::displayWidth(line));
        setImplicitWidth(std::max(
            1, maximumWidth + leftPadding() + rightPadding()));
        setImplicitHeight(std::max(
            1, static_cast<int>(m_editor.text().count('\n')) + 1
                + topPadding() + bottomPadding()));
        emit textChanged();
    }
    if (change.cursor)
        emit cursorPositionChanged();
    if (change.selection)
        emit selectionChanged();
    if (change.any()) {
        ensureCursorVisible();
        markDirty();
    }
}

void TextArea::setText(const QString& text) {
    applyEditorChange(m_editor.setText(text));
}

void TextArea::setColor(Color color) {
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
    return m_editor.selectionStart();
}

int TextArea::selectionEnd() const {
    return m_editor.selectionEnd();
}

QString TextArea::selectedText() const {
    return m_editor.selectedText();
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
    const int maximum = std::max(
        0, static_cast<int>(m_editor.text().count('\n')) + 1
            - std::max(1, height() - topPadding() - bottomPadding()));
    const int clamped = std::clamp(y, 0, maximum);
    if (m_contentY == clamped) return;
    m_contentY = clamped;
    emit contentYChanged();
    markDirty();
}

void TextArea::selectAll() {
    applyEditorChange(m_editor.selectAll());
}

void TextArea::clearSelection() {
    applyEditorChange(m_editor.clearSelection());
}

void TextArea::copy() {
    if (!m_editor.hasSelection()) return;
    if (Terminal *term = terminal())
        term->copyToClipboard(m_editor.selectedText());
}

void TextArea::cut() {
    if (m_readOnly || !m_editor.hasSelection()) return;
    copy();
    applyEditorChange(m_editor.deleteSelection());
}

void TextArea::paste(const QString &text) {
    if (m_readOnly || text.isEmpty()) return;
    applyEditorChange(m_editor.replaceSelection(text));
}

void TextArea::undo() {
    if (m_readOnly) return;
    applyEditorChange(m_editor.undo());
}

void TextArea::redo() {
    if (m_readOnly) return;
    applyEditorChange(m_editor.redo());
}

void TextArea::lineColumnForPosition(int position, int &line, int &column) const {
    const int start = lineStart(position);
    line = m_editor.text().left(start).count('\n');
    column = UnicodeText::displayWidth(
        m_editor.text().mid(start, position - start));
}

int TextArea::positionForLineColumn(int line, int column) const {
    const QStringList lines = m_editor.text().split('\n');
    if (lines.isEmpty()) return 0;
    line = std::clamp(line, 0, static_cast<int>(lines.size()) - 1);
    int position = 0;
    for (int index = 0; index < line; ++index)
        position += lines[index].size() + 1;
    return position + TextEditorState::positionForColumn(lines[line], column);
}

int TextArea::lineStart(int position) const {
    const int newline = m_editor.text().lastIndexOf(
        '\n', std::max(0, position - 1));
    return newline < 0 ? 0 : newline + 1;
}

int TextArea::lineEnd(int position) const {
    const int newline = m_editor.text().indexOf('\n', position);
    return newline < 0 ? m_editor.text().size() : newline;
}

void TextArea::moveCursor(int position, bool extendSelection) {
    applyEditorChange(m_editor.moveCursor(position, extendSelection));
}

void TextArea::ensureCursorVisible() {
    int line = 0;
    int column = 0;
    lineColumnForPosition(m_editor.cursorPosition(), line, column);
    const int viewportWidth = std::max(
        1, width() - leftPadding() - rightPadding());
    const int viewportHeight = std::max(
        1, height() - topPadding() - bottomPadding());
    if (line < m_contentY) setContentY(line);
    else if (line >= m_contentY + viewportHeight)
        setContentY(line - viewportHeight + 1);
    if (column < m_contentX) setContentX(column);
    else if (column >= m_contentX + viewportWidth)
        setContentX(column - viewportWidth + 1);
}

void TextArea::keyPressEvent(KeyEvent* event) {
    if (!activeFocus() || !isEffectivelyEnabled()) return;
    const int key = event->key();
    const bool shift = event->modifiers() & Qt::ShiftModifier;
    const bool control = event->modifiers() & Qt::ControlModifier;

    if (key == Qt::Key_Tab || key == Qt::Key_Backtab)
        return;

    TextEditorState::Change change;
    if (control && key == Qt::Key_A) {
        change = m_editor.selectAll();
    } else if (control && key == Qt::Key_C) {
        if (!m_editor.hasSelection()) return;
        copy();
    } else if (control && key == Qt::Key_X) {
        if (m_readOnly || !m_editor.hasSelection()) return;
        copy();
        change = m_editor.deleteSelection();
    } else if (control && key == Qt::Key_Z && shift) {
        if (!m_readOnly) change = m_editor.redo();
    } else if (control && key == Qt::Key_Z) {
        if (!m_readOnly) change = m_editor.undo();
    } else if (control && key == Qt::Key_Y) {
        if (!m_readOnly) change = m_editor.redo();
    } else if (key == Qt::Key_Left) {
        const int position = m_editor.hasSelection() && !shift
            ? m_editor.selectionStart()
            : (control
                ? m_editor.previousWordPosition()
                : UnicodeText::previousBoundary(
                    m_editor.text(), m_editor.cursorPosition()));
        change = m_editor.moveCursor(position, shift);
    } else if (key == Qt::Key_Right) {
        const int position = m_editor.hasSelection() && !shift
            ? m_editor.selectionEnd()
            : (control
                ? (shift
                    ? m_editor.nextWordEndPosition()
                    : m_editor.nextWordPosition())
                : UnicodeText::nextBoundary(
                    m_editor.text(), m_editor.cursorPosition()));
        change = m_editor.moveCursor(position, shift);
    } else if (key == Qt::Key_Home) {
        change = m_editor.moveCursor(lineStart(m_editor.cursorPosition()), shift);
    } else if (key == Qt::Key_End) {
        change = m_editor.moveCursor(lineEnd(m_editor.cursorPosition()), shift);
    } else if (key == Qt::Key_Up || key == Qt::Key_Down) {
        int line = 0, column = 0;
        lineColumnForPosition(m_editor.cursorPosition(), line, column);
        change = m_editor.moveCursor(positionForLineColumn(
            line + (key == Qt::Key_Up ? -1 : 1), column), shift);
    } else if (key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
        int line = 0, column = 0;
        lineColumnForPosition(m_editor.cursorPosition(), line, column);
        const int page = std::max(
            1, height() - topPadding() - bottomPadding());
        change = m_editor.moveCursor(positionForLineColumn(
            line + (key == Qt::Key_PageUp ? -page : page), column), shift);
    } else if (!m_readOnly && key == Qt::Key_Backspace) {
        change = control
            ? m_editor.deletePreviousWord()
            : m_editor.deletePreviousCharacter();
    } else if (!m_readOnly && key == Qt::Key_Delete) {
        change = control
            ? m_editor.deleteNextWord()
            : m_editor.deleteNextCharacter();
    } else if (!m_readOnly && control && key == Qt::Key_W) {
        change = m_editor.deletePreviousWord();
    } else if (!m_readOnly && control && key == Qt::Key_U) {
        change = m_editor.deleteToStart();
    } else if (!m_readOnly && control && key == Qt::Key_K) {
        change = m_editor.deleteToEnd();
    } else if (!m_readOnly
               && (key == Qt::Key_Return || key == Qt::Key_Enter)) {
        change = m_editor.replaceSelection("\n");
    } else if (!m_readOnly && !control && !event->text().isEmpty()) {
        change = m_editor.replaceSelection(event->text());
    } else {
        return;
    }
    applyEditorChange(change);
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

void TextArea::paint(
    TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    const int viewportWidth = std::max(
        0, width() - leftPadding() - rightPadding());
    const int viewportHeight = std::max(
        0, height() - topPadding() - bottomPadding());
    const ClipRect contentClip = clipRect.intersected(
        ClipRect{absX + leftPadding(), absY + topPadding(),
                 viewportWidth, viewportHeight});

    const QString displayText = m_editor.text().isEmpty() && !activeFocus()
        ? m_placeholderText : m_editor.text();
    const uint32_t displayColor = m_resolvedColor;
    const TextAttribute displayAttributes = !isEffectivelyEnabled()
        || (m_editor.text().isEmpty() && !activeFocus())
        ? TextAttribute::Dim : TextAttribute::None;
    const QStringList lines = displayText.split('\n');
    int globalOffset = 0;
    for (int line = 0; line < m_contentY && line < lines.size(); ++line)
        globalOffset += lines[line].size() + 1;

    for (int visibleLine = 0; visibleLine < viewportHeight; ++visibleLine) {
        const int lineIndex = m_contentY + visibleLine;
        if (lineIndex >= lines.size()) break;
        const int y = absY + topPadding() + visibleLine;
        int displayColumn = 0;
        GraphemeIterator iterator(lines[lineIndex], /*sanitizeControls=*/true);
        GraphemeRef cluster;
        while (iterator.next(cluster)) {
            const int x = absX + leftPadding() + displayColumn - m_contentX;
            const int width = std::max(1, cluster.width);
            if (x >= absX + leftPadding()
                && x + width <= absX + leftPadding() + viewportWidth
                && contentClip.contains(x, y)
                && contentClip.contains(x + width - 1, y)) {
                buffer.setGrapheme(x, y, cluster.utf16, cluster.width, displayColor, 0, displayAttributes);
                const int clusterStart = globalOffset + cluster.utf16Start;
                const int clusterEnd = clusterStart + cluster.utf16Length();
                if (!m_editor.text().isEmpty()
                    && clusterEnd > selectionStart()
                    && clusterStart < selectionEnd()) {
                    for (int dx = 0; dx < width; ++dx) {
                        if (Cell *cell = buffer.tryCellAt(x + dx, y))
                            cell->attributes = cell->attributes
                                | TextAttribute::Reversed;
                    }
                }
            }
            displayColumn += cluster.width;
        }
        globalOffset += lines[lineIndex].size() + 1;
    }

    if (activeFocus()) {
        int line = 0, column = 0;
        lineColumnForPosition(m_editor.cursorPosition(), line, column);
        const int x = absX + leftPadding() + column - m_contentX;
        const int y = absY + topPadding() + line - m_contentY;
        if (contentClip.contains(x, y))
            if (Cell *cell = buffer.tryCellAt(x, y)) {
                if (cell->character().isEmpty()) cell->setCharacter(" ");
                cell->attributes = cell->attributes | TextAttribute::Reversed;
            }
    }
}

} // namespace qamui
