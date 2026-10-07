#include "TextInput.h"

#include "../engine/KeyEvent.h"
#include "../engine/MouseEvent.h"
#include "../primitives/ColorMapper.h"
#include "../primitives/UnicodeText.h"
#include "Terminal.h"

#include <algorithm>

namespace qtui {

TextInput::TextInput(QObject *parent) : Item(parent) {
    setFocusable(true);
    setClip(true);
    setImplicitWidth(10);
    setImplicitHeight(1);
    connect(this, &Item::widthChanged, this, &TextInput::ensureCursorVisible);
    connect(this, &Item::paddingChanged, this, &TextInput::ensureCursorVisible);
    connect(this, &Item::paddingChanged, this, &TextInput::updateImplicitWidth);

    connect(&m_blinkTimer, &QTimer::timeout, this, [this]() {
        m_blinkState = !m_blinkState;
        markDirty();
    });

    connect(this, &Item::activeFocusChanged, this, [this]() {
        resetBlinkTimer();
        markDirty();
    });
}

TextInput::~TextInput() = default;

void TextInput::resetBlinkTimer() {
    m_blinkState = true;
    if (activeFocus() && m_cursorBlinking && m_cursorBlinkRate > 0) {
        m_blinkTimer.start(m_cursorBlinkRate);
    } else {
        m_blinkTimer.stop();
    }
}

TextInput::EditorStatus TextInput::editorStatus() const {
    return EditorStatus{acceptableInput(), canUndo(), canRedo()};
}

void TextInput::applyEditorChange(
    const TextEditorState::Change &change, const EditorStatus &before) {
    if (change.text) {
        updateImplicitWidth();
        emit textChanged();
    }
    if (change.cursor)
        emit cursorPositionChanged();
    if (change.selection)
        emit selectionChanged();
    if (before.acceptable != acceptableInput())
        emit acceptableInputChanged();
    if (before.canUndo != canUndo())
        emit canUndoChanged();
    if (before.canRedo != canRedo())
        emit canRedoChanged();
    if (change.any()) {
        ensureCursorVisible();
        resetBlinkTimer();
        markDirty();
    }
}

void TextInput::setText(const QString &text) {
    const EditorStatus before = editorStatus();
    const QString sanitized = TextEditorState::sanitizeSingleLine(text);
    applyEditorChange(
        m_editor.setText(TextEditorState::truncateToGraphemes(sanitized, m_maximumLength)),
        before);
}

void TextInput::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    emit colorChanged();
    markDirty();
}

void TextInput::setPlaceholderText(const QString &text) {
    const QString sanitized = TextEditorState::sanitizeSingleLine(text);
    if (m_placeholderText == sanitized) return;
    m_placeholderText = sanitized;
    updateImplicitWidth();
    emit placeholderTextChanged();
    markDirty();
}

void TextInput::setCursorPosition(int pos) {
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.moveCursor(pos), before);
}

void TextInput::setCursorShape(CursorShape shape) {
    if (m_cursorShape == shape) return;
    m_cursorShape = shape;
    emit cursorShapeChanged();
    markDirty();
}

void TextInput::setCursorColor(Color color) {
    if (m_cursorColor == color) return;
    m_cursorColor = color;
    if (!m_cursorColor.isValid()) {
        m_hasResolvedCursorColor = false;
        m_resolvedCursorColor = 0;
    } else {
        m_hasResolvedCursorColor = true;
        m_resolvedCursorColor = ColorMapper::fromString(m_cursorColor, 0xFFFFFFFF);
    }
    emit cursorColorChanged();
    markDirty();
}

void TextInput::setCursorBlinking(bool blinking) {
    if (m_cursorBlinking == blinking) return;
    m_cursorBlinking = blinking;
    resetBlinkTimer();
    emit cursorBlinkingChanged();
    markDirty();
}

void TextInput::setCursorBlinkRate(int rate) {
    const int validRate = std::max(50, rate);
    if (m_cursorBlinkRate == validRate) return;
    m_cursorBlinkRate = validRate;
    resetBlinkTimer();
    emit cursorBlinkRateChanged();
    markDirty();
}

void TextInput::setCursorVisible(bool visible) {
    if (m_cursorVisible == visible) return;
    m_cursorVisible = visible;
    emit cursorVisibleChanged();
    markDirty();
}

void TextInput::setCursorDelegate(const QString &delegate) {
    if (m_cursorDelegate == delegate) return;
    m_cursorDelegate = delegate;
    emit cursorDelegateChanged();
    markDirty();
}

void TextInput::setSelectionColor(Color color) {
    if (m_selectionColor == color) return;
    m_selectionColor = color;
    if (!m_selectionColor.isValid()) {
        m_hasResolvedSelectionColor = false;
        m_resolvedSelectionColor = 0;
    } else {
        m_hasResolvedSelectionColor = true;
        m_resolvedSelectionColor = ColorMapper::fromString(m_selectionColor, 0xFF0000FF);
    }
    emit selectionColorChanged();
    markDirty();
}

void TextInput::setSelectedTextColor(Color color) {
    if (m_selectedTextColor == color) return;
    m_selectedTextColor = color;
    if (!m_selectedTextColor.isValid()) {
        m_hasResolvedSelectedTextColor = false;
        m_resolvedSelectedTextColor = 0;
    } else {
        m_hasResolvedSelectedTextColor = true;
        m_resolvedSelectedTextColor = ColorMapper::fromString(m_selectedTextColor, 0xFFFFFFFF);
    }
    emit selectedTextColorChanged();
    markDirty();
}

void TextInput::setPlaceholderColor(Color color) {
    if (m_placeholderColor == color) return;
    m_placeholderColor = color;
    if (!m_placeholderColor.isValid()) {
        m_hasResolvedPlaceholderColor = false;
        m_resolvedPlaceholderColor = 0;
    } else {
        m_hasResolvedPlaceholderColor = true;
        m_resolvedPlaceholderColor = ColorMapper::fromString(m_placeholderColor,
                                                            Color::fromAnsiIndex(8).argb32());
    }
    emit placeholderColorChanged();
    markDirty();
}

void TextInput::setOverwriteMode(bool overwrite) {
    if (m_overwriteMode == overwrite) return;
    m_overwriteMode = overwrite;
    emit overwriteModeChanged();
    markDirty();
}

void TextInput::setAutoScroll(bool autoScroll) {
    if (m_autoScroll == autoScroll) return;
    m_autoScroll = autoScroll;
    emit autoScrollChanged();
}

void TextInput::setInputMask(const QString &mask) {
    if (m_inputMask == mask) return;
    m_inputMask = mask;
    emit inputMaskChanged();
    markDirty();
}

void TextInput::clear() {
    setText("");
    clearSelection();
}

void TextInput::setReadOnly(bool readOnly) {
    if (m_readOnly == readOnly) return;
    m_readOnly = readOnly;
    emit readOnlyChanged();
}

void TextInput::setContentX(int x) {
    const int clamped = std::max(0, x);
    if (m_contentX == clamped) return;
    m_contentX = clamped;
    emit contentXChanged();
    markDirty();
}

void TextInput::setMaximumLength(int length) {
    const int clamped = std::max(0, length);
    if (m_maximumLength == clamped) return;
    m_maximumLength = clamped;
    emit maximumLengthChanged();
    setText(m_editor.text());
}

void TextInput::setValidator(const QRegularExpression &validator) {
    if (m_validator == validator) return;
    const bool wasAcceptable = acceptableInput();
    m_validator = validator;
    emit validatorChanged();
    if (wasAcceptable != acceptableInput())
        emit acceptableInputChanged();
}

bool TextInput::acceptableInput() const {
    if (!m_validator.isValid() || m_validator.pattern().isEmpty())
        return true;
    const QRegularExpressionMatch match = m_validator.match(m_editor.text());
    return match.hasMatch() && match.capturedStart() == 0
        && match.capturedLength() == m_editor.text().size();
}

void TextInput::setEchoMode(EchoMode mode) {
    if (m_echoMode == mode) return;
    m_echoMode = mode;
    updateImplicitWidth();
    ensureCursorVisible();
    emit echoModeChanged();
    markDirty();
}

void TextInput::setEchoCharacter(const QString &character) {
    const QVector<Grapheme> clusters = UnicodeText::graphemes(character);
    const QString candidate = clusters.isEmpty()
        ? QString("*")
        : character.mid(clusters.first().utf16Start, clusters.first().utf16Length);
    const QString normalized = UnicodeText::displayWidth(candidate) > 0
        ? candidate : QString("*");
    if (m_echoCharacter == normalized) return;
    m_echoCharacter = normalized;
    updateImplicitWidth();
    ensureCursorVisible();
    emit echoCharacterChanged();
    markDirty();
}

void TextInput::select(int start, int end) {
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.select(start, end), before);
}

void TextInput::selectAll() {
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.selectAll(), before);
}

void TextInput::clearSelection() {
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.clearSelection(), before);
}

void TextInput::copy() {
    if (!m_editor.hasSelection()) return;
    if (Terminal *term = terminal())
        term->copyToClipboard(m_editor.selectedText());
}

void TextInput::cut() {
    if (m_readOnly || !m_editor.hasSelection()) return;
    copy();
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.deleteSelection(), before);
}

QString TextInput::constrainedReplacement(const QString &replacement) const {
    const QString sanitized = TextEditorState::sanitizeSingleLine(replacement);
    if (m_maximumLength <= 0)
        return sanitized;
    const int retained = TextEditorState::graphemeCount(m_editor.text())
        - TextEditorState::graphemeCount(m_editor.selectedText());
    const int available = m_maximumLength - retained;
    if (available <= 0)
        return {};
    return TextEditorState::truncateToGraphemes(sanitized, available);
}

void TextInput::paste(const QString &text) {
    if (m_readOnly || text.isEmpty()) return;
    const QString replacement = constrainedReplacement(text);
    if (replacement.isEmpty()) return;
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.replaceSelection(replacement), before);
}

void TextInput::undo() {
    if (m_readOnly) return;
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.undo(), before);
}

void TextInput::redo() {
    if (m_readOnly) return;
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.redo(), before);
}

QString TextInput::displayText() const {
    if (m_editor.text().isEmpty())
        return m_placeholderText;
    if (m_echoMode == NoEcho)
        return {};
    if (m_echoMode == Normal)
        return m_editor.text();

    QString masked;
    const int count = TextEditorState::graphemeCount(m_editor.text());
    for (int index = 0; index < count; ++index)
        masked.append(m_echoCharacter);
    return masked;
}

int TextInput::displayColumnForPosition(int position) const {
    if (m_echoMode == NoEcho)
        return 0;
    if (m_echoMode == Normal)
        return TextEditorState::columnForPosition(m_editor.text(), position);

    const int snapped = TextEditorState::snapToBoundary(m_editor.text(), position);
    int count = 0;
    GraphemeIterator iterator(m_editor.text());
    GraphemeRef cluster;
    while (iterator.next(cluster)) {
        if (cluster.utf16Start >= snapped)
            break;
        ++count;
    }
    return count * UnicodeText::displayWidth(m_echoCharacter);
}

int TextInput::positionForDisplayColumn(int column) const {
    if (m_echoMode == Normal)
        return TextEditorState::positionForColumn(m_editor.text(), column);
    if (m_echoMode == NoEcho)
        return 0;

    const int maskWidth = std::max(1, UnicodeText::displayWidth(m_echoCharacter));
    const int targetIndex = std::max(0, column) / maskWidth;
    GraphemeIterator iterator(m_editor.text());
    GraphemeRef cluster;
    int index = 0;
    while (iterator.next(cluster)) {
        if (index == targetIndex)
            return cluster.utf16Start;
        ++index;
    }
    return m_editor.text().size();
}

int TextInput::positionAtMouse(int x) const {
    return positionForDisplayColumn(m_contentX + std::max(0, x - leftPadding()));
}

void TextInput::ensureCursorVisible() {
    if (!m_autoScroll)
        return;
    const int viewportWidth = std::max(1, width() - leftPadding() - rightPadding());
    const int column = displayColumnForPosition(m_editor.cursorPosition());
    if (column < m_contentX)
        setContentX(column);
    else if (column >= m_contentX + viewportWidth)
        setContentX(column - viewportWidth + 1);
    else if (UnicodeText::displayWidth(displayText()) < viewportWidth)
        setContentX(0);
}

void TextInput::updateImplicitWidth() {
    setImplicitWidth(std::max(
        10, UnicodeText::displayWidth(displayText()) + leftPadding() + rightPadding()));
}

void TextInput::keyPressEvent(KeyEvent* event) {
    const int key = event->key();
    const bool shift = event->modifiers() & Qt::ShiftModifier;
    const bool control = event->modifiers() & Qt::ControlModifier;
    const bool alt = event->modifiers() & Qt::AltModifier;
    const bool wordModifier = control || alt;

    if (key == Qt::Key_Tab || key == Qt::Key_Backtab)
        return;

    const EditorStatus before = editorStatus();
    TextEditorState::Change change;
    if (control && key == Qt::Key_A) {
        change = m_editor.selectAll();
    } else if (control && key == Qt::Key_C) {
        if (!m_editor.hasSelection()) return;
        copy();
    } else if (control && key == Qt::Key_X) {
        if (!m_editor.hasSelection()) return;
        if (!m_readOnly) {
            copy();
            change = m_editor.deleteSelection();
        }
    } else if (control && key == Qt::Key_Z && shift) {
        if (!m_readOnly) change = m_editor.redo();
    } else if (control && key == Qt::Key_Z) {
        if (!m_readOnly) change = m_editor.undo();
    } else if (control && key == Qt::Key_Y) {
        if (!m_readOnly) change = m_editor.redo();
    } else if (key == Qt::Key_Left) {
        int position = m_editor.hasSelection() && !shift
            ? m_editor.selectionStart()
            : (wordModifier
                ? (m_echoMode == Normal ? m_editor.previousWordPosition() : 0)
                : UnicodeText::previousBoundary(
                    m_editor.text(), m_editor.cursorPosition()));
        change = m_editor.moveCursor(position, shift);
    } else if (key == Qt::Key_Right) {
        int position = m_editor.hasSelection() && !shift
            ? m_editor.selectionEnd()
            : (wordModifier
                ? (m_echoMode == Normal
                    ? (shift
                        ? m_editor.nextWordEndPosition()
                        : m_editor.nextWordPosition())
                    : m_editor.text().size())
                : UnicodeText::nextBoundary(
                    m_editor.text(), m_editor.cursorPosition()));
        change = m_editor.moveCursor(position, shift);
    } else if (key == Qt::Key_Home || (control && key == Qt::Key_A)) {
        change = m_editor.moveCursor(0, shift);
    } else if (key == Qt::Key_End || (control && key == Qt::Key_E)) {
        change = m_editor.moveCursor(m_editor.text().size(), shift);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        emit accepted();
        emit editingFinished();
    } else if (key == Qt::Key_Backspace || (control && key == Qt::Key_H)) {
        if (!m_readOnly)
            change = wordModifier && key == Qt::Key_Backspace
                ? m_editor.deletePreviousWord() : m_editor.deletePreviousCharacter();
    } else if (key == Qt::Key_Delete || (control && key == Qt::Key_D)) {
        if (!m_readOnly)
            change = wordModifier && key == Qt::Key_Delete
                ? m_editor.deleteNextWord() : m_editor.deleteNextCharacter();
    } else if (key == Qt::Key_Insert) {
        setOverwriteMode(!m_overwriteMode);
        event->setAccepted(true);
        return;
    } else if (control && key == Qt::Key_W) {
        if (!m_readOnly) change = m_editor.deletePreviousWord();
    } else if (control && key == Qt::Key_U) {
        if (!m_readOnly) change = m_editor.deleteToStart();
    } else if (control && key == Qt::Key_K) {
        if (!m_readOnly) change = m_editor.deleteToEnd();
    } else if (!control && !alt && !event->text().isEmpty()) {
        if (!m_readOnly) {
            if (m_overwriteMode && !m_editor.hasSelection()
                && m_editor.cursorPosition() < m_editor.text().size()) {
                const int currentPos = m_editor.cursorPosition();
                const int nextPos = UnicodeText::nextBoundary(m_editor.text(), currentPos);
                m_editor.select(currentPos, nextPos);
            }
            const QString replacement = constrainedReplacement(event->text());
            if (!replacement.isEmpty())
                change = m_editor.replaceSelection(replacement);
        }
    } else {
        return;
    }

    applyEditorChange(change, before);
    event->setAccepted(true);
}

void TextInput::mousePressEvent(MouseEvent *event) {
    setFocus(true);
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.moveCursor(positionAtMouse(event->x())), before);
    event->setAccepted(true);
}

void TextInput::mouseMoveEvent(MouseEvent *event) {
    const EditorStatus before = editorStatus();
    applyEditorChange(m_editor.moveCursor(positionAtMouse(event->x()), true), before);
    event->setAccepted(true);
}

void TextInput::paint(
    TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!m_visible) return;

    const int viewportWidth = std::max(0, width() - leftPadding() - rightPadding());
    const int viewportHeight = std::max(0, height() - topPadding() - bottomPadding());
    if (viewportWidth == 0 || viewportHeight == 0)
        return;

    const int y = absY + topPadding();
    const int contentLeft = absX + leftPadding();
    const int contentRight = contentLeft + viewportWidth;
    const ClipRect contentClip = clipRect.intersected(
        ClipRect{contentLeft, y, viewportWidth, 1});
    const bool placeholder = m_editor.text().isEmpty() && !m_placeholderText.isEmpty();
    const uint32_t displayColor = placeholder && m_hasResolvedPlaceholderColor
        ? m_resolvedPlaceholderColor : m_resolvedColor;
    const TextAttribute displayAttributes = !isEffectivelyEnabled()
        || (placeholder && !m_hasResolvedPlaceholderColor)
        ? TextAttribute::Dim : TextAttribute::None;

    if (placeholder) {
        int column = 0;
        GraphemeIterator iterator(m_placeholderText, /*sanitizeControls=*/true);
        GraphemeRef cluster;
        while (iterator.next(cluster)) {
            const int x = contentLeft + column - m_contentX;
            const int width = std::max(1, cluster.width);
            if (x >= contentLeft && x + width <= contentRight
                && contentClip.contains(x, y) && contentClip.contains(x + width - 1, y))
                buffer.setGrapheme(x, y, cluster.utf16, cluster.width, displayColor, 0, displayAttributes);
            column += cluster.width;
        }
    } else if (m_echoMode != NoEcho) {
        int column = 0;
        const bool masked = m_echoMode == Password;
        const int maskWidth = masked ? std::max(1, UnicodeText::displayWidth(m_echoCharacter)) : 1;
        GraphemeIterator iterator(m_editor.text(), /*sanitizeControls=*/true);
        GraphemeRef cluster;
        while (iterator.next(cluster)) {
            const int width = masked ? maskWidth : std::max(1, cluster.width);
            const int x = contentLeft + column - m_contentX;
            if (x >= contentLeft && x + width <= contentRight
                && contentClip.contains(x, y) && contentClip.contains(x + width - 1, y)) {
                if (masked)
                    buffer.writeText(x, y, m_echoCharacter, displayColor, 0, displayAttributes);
                else
                    buffer.setGrapheme(x, y, cluster.utf16, cluster.width, displayColor, 0, displayAttributes);
                const int clusterStart = cluster.utf16Start;
                const int clusterEnd = clusterStart + cluster.utf16Length();
                if (clusterEnd > selectionStart() && clusterStart < selectionEnd()) {
                    for (int dx = 0; dx < width; ++dx) {
                        Cell *selectionCell = buffer.tryCellAt(x + dx, y);
                        if (!selectionCell)
                            continue;
                        Cell &selCell = *selectionCell;
                        if (m_hasResolvedSelectionColor) {
                            selCell.bgColor = m_resolvedSelectionColor;
                            if (m_hasResolvedSelectedTextColor) {
                                selCell.fgColor = m_resolvedSelectedTextColor;
                            }
                        } else {
                            if (m_hasResolvedSelectedTextColor) {
                                selCell.fgColor = m_resolvedSelectedTextColor;
                            }
                            selCell.attributes = selCell.attributes | TextAttribute::Reversed;
                        }
                    }
                }
            }
            column += width;
        }
    }

    if (activeFocus() && m_cursorVisible && m_blinkState) {
        const int x = contentLeft
            + displayColumnForPosition(m_editor.cursorPosition()) - m_contentX;
        if (contentClip.contains(x, y)) {
            Cell *cursorCell = buffer.tryCellAt(x, y);
            if (!cursorCell)
                return;
            Cell &cell = *cursorCell;
            const bool isSpaceOrEmpty = cell.character().isEmpty() || cell.character() == " ";

            if (!m_cursorDelegate.isEmpty()) {
                if (isSpaceOrEmpty) {
                    cell.setCharacter(m_cursorDelegate.toUtf8());
                }
                if (m_hasResolvedCursorColor) {
                    if (isSpaceOrEmpty) {
                        cell.fgColor = m_resolvedCursorColor;
                    } else {
                        cell.bgColor = m_resolvedCursorColor;
                        cell.fgColor = 0x000000FF;
                    }
                } else {
                    cell.attributes = cell.attributes | TextAttribute::Reversed;
                }
            } else {
                switch (m_cursorShape) {
                case Block: {
                    if (isSpaceOrEmpty)
                        cell.setCharacter(" ");
                    if (m_hasResolvedCursorColor) {
                        cell.bgColor = m_resolvedCursorColor;
                        cell.fgColor = 0x000000FF;
                    } else {
                        cell.attributes = cell.attributes | TextAttribute::Reversed;
                    }
                    break;
                }
                case Underline: {
                    if (isSpaceOrEmpty)
                        cell.setCharacter("_");
                    if (m_hasResolvedCursorColor) {
                        cell.fgColor = m_resolvedCursorColor;
                    }
                    cell.attributes = cell.attributes | TextAttribute::Underline;
                    break;
                }
                case Beam: {
                    if (isSpaceOrEmpty) {
                        cell.setCharacter("│");
                        if (m_hasResolvedCursorColor) {
                            cell.fgColor = m_resolvedCursorColor;
                        }
                    } else {
                        if (m_hasResolvedCursorColor) {
                            cell.bgColor = m_resolvedCursorColor;
                            cell.fgColor = 0x000000FF;
                        } else {
                            cell.attributes = cell.attributes | TextAttribute::Reversed;
                        }
                    }
                    break;
                }
                case HollowBlock: {
                    if (isSpaceOrEmpty) {
                        cell.setCharacter("▯");
                        if (m_hasResolvedCursorColor) {
                            cell.fgColor = m_resolvedCursorColor;
                        }
                    } else {
                        if (m_hasResolvedCursorColor) {
                            cell.fgColor = m_resolvedCursorColor;
                        }
                        cell.attributes = cell.attributes | TextAttribute::Bold;
                    }
                    break;
                }
                }
            }
        }
    }
}

} // namespace qtui
