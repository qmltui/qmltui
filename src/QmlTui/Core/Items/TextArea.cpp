#include <QmlTui/Core/Items/TextArea.h>
#include <QmlTui/Core/Items/TextEditorState.h>
#include <QmlTui/Core/Engine/Clock_p.h>

#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/TextCursorPainter.h>
#include <QStringTokenizer>
#include <QStringView>

#include <algorithm>
#include <memory>
#include <set>
#include <utility>

namespace qmltui {

static_assert(int(TextArea::Block) == int(detail::TextCursorShape::Block) &&
              int(TextArea::Underline) == int(detail::TextCursorShape::Underline) &&
              int(TextArea::Beam) == int(detail::TextCursorShape::Beam) &&
              int(TextArea::HollowBlock) == int(detail::TextCursorShape::HollowBlock));

TextArea::TextArea(QObject* parent) : Item(parent), m_editor(std::make_unique<TextEditorState>()) {
    setItemFlag(ItemFlag::BoundedSelfPaint);
    setFocusPolicy(Qt::StrongFocus);
    setClip(true);
    setImplicitWidth(20);
    setImplicitHeight(5);

    m_blinkTimer = new ClockTimer(this);
    connect(m_blinkTimer, &ClockTimer::timeout, this, [this]() {
        m_blinkState = !m_blinkState;
        markDirty();
    });

    connect(this, &Item::activeFocusChanged, this, [this]() {
        resetBlinkTimer();
        markDirty();
    });
    connect(this, &Item::parentChanged, this, &TextArea::updateTerminalConnection);
    connect(this, &Item::ancestryChanged, this, &TextArea::updateTerminalConnection);
}

TextArea::~TextArea() = default;

QString TextArea::text() const {
    return m_editor->text();
}

int TextArea::cursorPosition() const {
    return m_editor->cursorPosition();
}

bool TextArea::canUndo() const {
    return m_editor->canUndo();
}

bool TextArea::canRedo() const {
    return m_editor->canRedo();
}

void TextArea::updateTerminalConnection() {
    Terminal* term = terminal();
    if (m_observedTerminal == term) return;
    if (m_observedTerminal) {
        disconnect(m_appActiveConnection);
    }
    m_observedTerminal = term;
    if (m_observedTerminal) {
        m_appActiveConnection =
            connect(m_observedTerminal, &Terminal::applicationActiveChanged, this, [this]() {
                resetBlinkTimer();
                markDirty();
            });
    }
}

void TextArea::resetBlinkTimer() {
    updateTerminalConnection();
    m_blinkState = true;
    const bool appActive = !m_observedTerminal || m_observedTerminal->applicationActive();
    if (activeFocus() && appActive && m_cursorBlinking && m_cursorBlinkRate > 0) {
        m_blinkTimer->start(m_cursorBlinkRate);
    } else {
        m_blinkTimer->stop();
    }
}

void TextArea::applyEditorChange(const TextEditorState::Change& change) {
    if (change.text) {
        const QString& currentText = m_editor->text();
        m_lineOffsets.clear();
        m_lineOffsets.reserve(currentText.size() / 32 + 1);
        m_lineOffsets.append(0);
        const QChar* chars = currentText.unicode();
        const int len = static_cast<int>(currentText.size());
        int lineBegin = 0;
        int maximumWidth = 0;
        for (int i = 0; i < len; ++i) {
            if (chars[i] == u'\n') {
                maximumWidth =
                    (std::max)(maximumWidth, UnicodeText::displayWidth(
                                                 QStringView(chars + lineBegin, i - lineBegin),
                                                 /*sanitizeControls=*/true));
                lineBegin = i + 1;
                m_lineOffsets.append(lineBegin);
            }
        }
        maximumWidth =
            (std::max)(maximumWidth,
                       UnicodeText::displayWidth(QStringView(chars + lineBegin, len - lineBegin),
                                                 /*sanitizeControls=*/true));
        setImplicitWidth((std::max)(1, maximumWidth + leftPadding() + rightPadding()));
        setImplicitHeight(
            (std::max)(1, static_cast<int>(m_lineOffsets.size()) + topPadding() + bottomPadding()));
        ++m_textRevision;
        m_graphemeBoundariesValid = false;
        m_graphemeBoundaries.clear();
        if (!m_highlightLayers.isEmpty()) {
            m_highlightLayers.clear();
            m_composedRuns.clear();
            emit highlightsChanged();
        }
        emit textRevisionChanged();
        emit textChanged();
    }
    if (change.cursor) emit cursorPositionChanged();
    if (change.selection) emit selectionChanged();
    const bool currentCanUndo = canUndo();
    if (m_lastCanUndo != currentCanUndo) {
        m_lastCanUndo = currentCanUndo;
        emit canUndoChanged();
    }
    const bool currentCanRedo = canRedo();
    if (m_lastCanRedo != currentCanRedo) {
        m_lastCanRedo = currentCanRedo;
        emit canRedoChanged();
    }
    if (change.any()) {
        ensureCursorVisible();
        resetBlinkTimer();
        markDirty();
    }
}

void TextArea::setText(const QString& text) {
    applyEditorChange(m_editor->setText(text));
}

void TextArea::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = Color::resolve(m_color, Color::DefaultTag);
    emit colorChanged();
    markDirty();
}

void TextArea::setPlaceholderText(const QString& text) {
    if (m_placeholderText == text) return;
    m_placeholderText = text;
    emit placeholderTextChanged();
    markDirty();
}

void TextArea::setCursorPosition(int pos) {
    moveCursor(pos, false);
}

int TextArea::selectionStart() const {
    return m_editor->selectionStart();
}

int TextArea::selectionEnd() const {
    return m_editor->selectionEnd();
}

QString TextArea::selectedText() const {
    return m_editor->selectedText();
}

void TextArea::setReadOnly(bool readOnly) {
    if (m_readOnly == readOnly) return;
    m_readOnly = readOnly;
    emit readOnlyChanged();
}

void TextArea::setContentX(int x) {
    const int clamped = (std::max)(0, x);
    if (m_contentX == clamped) return;
    m_contentX = clamped;
    emit contentXChanged();
    markDirty();
}

void TextArea::setContentY(int y) {
    const int maximum = (std::max)(0, static_cast<int>(m_lineOffsets.size()) -
                                          (std::max)(1, height() - topPadding() - bottomPadding()));
    const int clamped = std::clamp(y, 0, maximum);
    if (m_contentY == clamped) return;
    m_contentY = clamped;
    emit contentYChanged();
    markDirty();
}

void TextArea::setCursorShape(CursorShape shape) {
    if (m_cursorShape == shape) return;
    m_cursorShape = shape;
    emit cursorShapeChanged();
    markDirty();
}

void TextArea::setCursorColor(Color color) {
    if (m_cursorColor == color) return;
    m_cursorColor = color;
    const std::optional<uint32_t> resolved = detail::resolveCursorColor(m_cursorColor);
    m_hasResolvedCursorColor = resolved.has_value();
    m_resolvedCursorColor = resolved.value_or(0);
    emit cursorColorChanged();
    markDirty();
}

void TextArea::setCursorBlinking(bool blinking) {
    if (m_cursorBlinking == blinking) return;
    m_cursorBlinking = blinking;
    resetBlinkTimer();
    emit cursorBlinkingChanged();
    markDirty();
}

void TextArea::setCursorBlinkRate(int rate) {
    const int validRate = (std::max)(50, rate);
    if (m_cursorBlinkRate == validRate) return;
    m_cursorBlinkRate = validRate;
    resetBlinkTimer();
    emit cursorBlinkRateChanged();
    markDirty();
}

void TextArea::setCursorVisible(bool visible) {
    if (m_cursorVisible == visible) return;
    m_cursorVisible = visible;
    emit cursorVisibleChanged();
    markDirty();
}

void TextArea::setSelectionColor(Color color) {
    if (m_selectionColor == color) return;
    m_selectionColor = color;
    const std::optional<uint32_t> resolved = detail::resolveCursorColor(m_selectionColor);
    m_hasResolvedSelectionColor = resolved.has_value();
    m_resolvedSelectionColor = resolved.value_or(0);
    emit selectionColorChanged();
    markDirty();
}

void TextArea::setSelectedTextColor(Color color) {
    if (m_selectedTextColor == color) return;
    m_selectedTextColor = color;
    const std::optional<uint32_t> resolved = detail::resolveCursorColor(m_selectedTextColor);
    m_hasResolvedSelectedTextColor = resolved.has_value();
    m_resolvedSelectedTextColor = resolved.value_or(0);
    emit selectedTextColorChanged();
    markDirty();
}

void TextArea::setTabChangesFocus(bool tabChangesFocus) {
    if (m_tabChangesFocus == tabChangesFocus) return;
    m_tabChangesFocus = tabChangesFocus;
    emit tabChangesFocusChanged();
}

void TextArea::setTabStopDistance(int distance) {
    distance = (std::max)(1, distance);
    if (m_tabStopDistance == distance) return;
    m_tabStopDistance = distance;
    emit tabStopDistanceChanged();
}

void TextArea::setShowWhitespace(bool show) {
    if (m_showWhitespace == show) return;
    m_showWhitespace = show;
    emit showWhitespaceChanged();
    markDirty();
}

void TextArea::select(int start, int end) {
    applyEditorChange(m_editor->select(start, end));
}

void TextArea::selectAll() {
    applyEditorChange(m_editor->selectAll());
}

void TextArea::clearSelection() {
    applyEditorChange(m_editor->clearSelection());
}

void TextArea::copy() {
    if (!m_editor->hasSelection()) return;
    if (Terminal* term = terminal()) term->copyToClipboard(m_editor->selectedText());
}

void TextArea::cut() {
    if (m_readOnly || !m_editor->hasSelection()) return;
    copy();
    applyEditorChange(m_editor->deleteSelection());
}

void TextArea::paste(const QString& text) {
    if (m_readOnly || text.isEmpty()) return;
    m_editor->breakUndoCoalescing();
    applyEditorChange(m_editor->replaceSelection(text));
}

void TextArea::undo() {
    if (m_readOnly) return;
    applyEditorChange(m_editor->undo());
}

void TextArea::redo() {
    if (m_readOnly) return;
    applyEditorChange(m_editor->redo());
}

QStringView TextArea::lineViewAt(int line) const {
    const QString& currentText = m_editor->text();
    const int start = m_lineOffsets[line];
    const int end = (line + 1 < m_lineOffsets.size()) ? (m_lineOffsets[line + 1] - 1)
                                                      : static_cast<int>(currentText.size());
    return QStringView(currentText).sliced(start, end - start);
}

void TextArea::lineColumnForPosition(int position, int& line, int& column) const {
    const QString& currentText = m_editor->text();
    const int clamped = std::clamp(position, 0, static_cast<int>(currentText.size()));
    const auto it = std::upper_bound(m_lineOffsets.cbegin(), m_lineOffsets.cend(), clamped);
    line = static_cast<int>(std::distance(m_lineOffsets.cbegin(), it)) - 1;
    const int start = m_lineOffsets[line];
    column = UnicodeText::displayWidth(QStringView(currentText).sliced(start, clamped - start),
                                       /*sanitizeControls=*/true);
}

int TextArea::positionForLineColumn(int line, int column) const {
    const QString& currentText = m_editor->text();
    if (currentText.isEmpty()) return 0;
    const int clampedLine = std::clamp(line, 0, static_cast<int>(m_lineOffsets.size()) - 1);
    const int start = m_lineOffsets[clampedLine];
    return start + TextEditorState::positionForColumn(lineViewAt(clampedLine), column,
                                                      /*sanitizeControls=*/true);
}

int TextArea::lineStart(int position) const {
    const int clamped = std::clamp(position, 0, static_cast<int>(m_editor->text().size()));
    const auto it = std::upper_bound(m_lineOffsets.cbegin(), m_lineOffsets.cend(), clamped);
    const int line = static_cast<int>(std::distance(m_lineOffsets.cbegin(), it)) - 1;
    return m_lineOffsets[line];
}

int TextArea::lineEnd(int position) const {
    const QString& currentText = m_editor->text();
    const int clamped = std::clamp(position, 0, static_cast<int>(currentText.size()));
    const auto it = std::upper_bound(m_lineOffsets.cbegin(), m_lineOffsets.cend(), clamped);
    const int line = static_cast<int>(std::distance(m_lineOffsets.cbegin(), it)) - 1;
    return (line + 1 < m_lineOffsets.size()) ? (m_lineOffsets[line + 1] - 1)
                                             : static_cast<int>(currentText.size());
}

void TextArea::moveCursor(int position, bool extendSelection) {
    applyEditorChange(m_editor->moveCursor(position, extendSelection));
}

void TextArea::ensureCursorVisible() {
    int line = 0;
    int column = 0;
    lineColumnForPosition(m_editor->cursorPosition(), line, column);
    const int viewportWidth = (std::max)(1, width() - leftPadding() - rightPadding());
    const int viewportHeight = (std::max)(1, height() - topPadding() - bottomPadding());
    if (line < m_contentY)
        setContentY(line);
    else if (line >= m_contentY + viewportHeight)
        setContentY(line - viewportHeight + 1);
    if (column < m_contentX)
        setContentX(column);
    else if (column >= m_contentX + viewportWidth)
        setContentX(column - viewportWidth + 1);
}

void TextArea::keyPressEvent(KeyEvent* event) {
    if (!activeFocus() || !isEffectivelyEnabled()) return;
    const int key = event->key();
    const bool shift = event->modifiers() & Qt::ShiftModifier;
    const bool control = event->modifiers() & Qt::ControlModifier;
    const bool alt = event->modifiers() & Qt::AltModifier;

    if (m_tabChangesFocus && (key == Qt::Key_Tab || key == Qt::Key_Backtab)) return;

    if (key == Qt::Key_Tab) {
        if (!m_readOnly) {
            int line = 0, column = 0;
            const int anchorPos =
                m_editor->hasSelection() ? m_editor->selectionStart() : m_editor->cursorPosition();
            lineColumnForPosition(anchorPos, line, column);
            int spacesCount = m_tabStopDistance - (column % m_tabStopDistance);
            if (spacesCount <= 0) spacesCount = m_tabStopDistance;
            applyEditorChange(m_editor->replaceSelection(QString(spacesCount, u' ')));
        }
        event->setAccepted(true);
        return;
    }

    if (key == Qt::Key_Backtab) {
        if (!m_readOnly && !m_editor->hasSelection()) {
            int line = 0, column = 0;
            lineColumnForPosition(m_editor->cursorPosition(), line, column);
            if (column > 0) {
                int spacesToRemove = column % m_tabStopDistance;
                if (spacesToRemove == 0) spacesToRemove = m_tabStopDistance;
                const int pos = m_editor->cursorPosition();
                const int startPos = lineStart(pos);
                int count = 0;
                while (pos - count > startPos && count < spacesToRemove &&
                       m_editor->text().at(pos - count - 1) == u' ') {
                    ++count;
                }
                if (count > 0) {
                    m_editor->select(pos - count, pos);
                    applyEditorChange(m_editor->deleteSelection());
                    event->setAccepted(true);
                    return;
                }
            }
        }
        return;
    }

    TextEditorState::Change change;
    if (control && key == Qt::Key_A) {
        change = m_editor->selectAll();
    } else if (control && key == Qt::Key_C) {
        if (!m_editor->hasSelection()) return;
        copy();
    } else if (control && key == Qt::Key_X) {
        if (m_readOnly || !m_editor->hasSelection()) return;
        copy();
        change = m_editor->deleteSelection();
    } else if (control && key == Qt::Key_V) {
        if (!m_readOnly) {
            if (Terminal* term = terminal()) {
                if (term->clipboardMode() != Terminal::ClipboardMode::Disabled) {
                    if (TerminalEngine* eng = term->engine()) {
                        auto conn = std::make_shared<QMetaObject::Connection>();
                        *conn = connect(eng, &TerminalEngine::clipboardReadFinished, this,
                                        [this, conn](const QString& text, const QString& code) {
                                            disconnect(*conn);
                                            if (code.isEmpty() && !text.isEmpty()) paste(text);
                                        });
                        if (!eng->requestClipboardText(2000).isEmpty()) disconnect(*conn);
                    }
                }
            }
        }
        event->setAccepted(true);
        return;
    } else if (control && key == Qt::Key_Z && shift) {
        if (!m_readOnly) change = m_editor->redo();
    } else if (control && key == Qt::Key_Z) {
        if (!m_readOnly) change = m_editor->undo();
    } else if (control && key == Qt::Key_Y) {
        if (!m_readOnly) change = m_editor->redo();
    } else if (key == Qt::Key_Left) {
        const int position = m_editor->hasSelection() && !shift
                                 ? m_editor->selectionStart()
                                 : (control ? m_editor->previousWordPosition()
                                            : UnicodeText::previousBoundary(
                                                  m_editor->text(), m_editor->cursorPosition()));
        change = m_editor->moveCursor(position, shift);
    } else if (key == Qt::Key_Right) {
        const int position =
            m_editor->hasSelection() && !shift
                ? m_editor->selectionEnd()
                : (control
                       ? (shift ? m_editor->nextWordEndPosition() : m_editor->nextWordPosition())
                       : UnicodeText::nextBoundary(m_editor->text(), m_editor->cursorPosition()));
        change = m_editor->moveCursor(position, shift);
    } else if (key == Qt::Key_Home) {
        change = m_editor->moveCursor(lineStart(m_editor->cursorPosition()), shift);
    } else if (key == Qt::Key_End) {
        change = m_editor->moveCursor(lineEnd(m_editor->cursorPosition()), shift);
    } else if (key == Qt::Key_Up || key == Qt::Key_Down) {
        int line = 0, column = 0;
        lineColumnForPosition(m_editor->cursorPosition(), line, column);
        change = m_editor->moveCursor(
            positionForLineColumn(line + (key == Qt::Key_Up ? -1 : 1), column), shift);
    } else if (key == Qt::Key_PageUp || key == Qt::Key_PageDown) {
        int line = 0, column = 0;
        lineColumnForPosition(m_editor->cursorPosition(), line, column);
        const int page = (std::max)(1, height() - topPadding() - bottomPadding());
        change = m_editor->moveCursor(
            positionForLineColumn(line + (key == Qt::Key_PageUp ? -page : page), column), shift);
    } else if (!m_readOnly && key == Qt::Key_Backspace) {
        change = control ? m_editor->deletePreviousWord() : m_editor->deletePreviousCharacter();
    } else if (!m_readOnly && key == Qt::Key_Delete) {
        change = control ? m_editor->deleteNextWord() : m_editor->deleteNextCharacter();
    } else if (!m_readOnly && control && key == Qt::Key_W) {
        change = m_editor->deletePreviousWord();
    } else if (!m_readOnly && control && key == Qt::Key_U) {
        change = m_editor->deleteToStart();
    } else if (!m_readOnly && control && key == Qt::Key_K) {
        change = m_editor->deleteToEnd();
    } else if (control && (key == Qt::Key_Return || key == Qt::Key_Enter)) {
        emit accepted();
        event->setAccepted(true);
        return;
    } else if (!m_readOnly && (key == Qt::Key_Return || key == Qt::Key_Enter)) {
        change = m_editor->replaceSelection("\n");
    } else if (!m_readOnly && !control && !alt && !event->text().isEmpty()) {
        change = m_editor->replaceSelection(event->text());
    } else {
        return;
    }
    applyEditorChange(change);
    event->setAccepted(true);
}

void TextArea::mousePressEvent(MouseEvent* event) {
    setFocus(true);
    const int pos = positionAtMouse(event->x(), event->y());
    const qint64 now = Clock::instance()->elapsed();
    TextEditorState::Change change = m_editor->moveCursor(pos);
    if (event->button() == Qt::LeftButton) {
        if (m_lastClickMs >= 0 && now - m_lastClickMs <= 400 && pos == m_lastClickPos) {
            int start = 0;
            int end = 0;
            if (pos < m_editor->text().size() && !m_editor->text()[pos].isSpace()) {
                end = m_editor->nextWordEndPosition();
                m_editor->moveCursor(end);
                start = m_editor->previousWordPosition();
            } else {
                start = m_editor->previousWordPosition();
                m_editor->moveCursor(start);
                end = m_editor->nextWordEndPosition();
            }
            m_editor->moveCursor(pos);
            const TextEditorState::Change selChange = m_editor->select(start, end);
            change.text = change.text || selChange.text;
            change.cursor = change.cursor || selChange.cursor;
            change.selection = change.selection || selChange.selection;
            m_lastClickMs = -1;
            m_lastClickPos = -1;
        } else {
            m_lastClickMs = now;
            m_lastClickPos = pos;
        }
    } else {
        m_lastClickMs = -1;
        m_lastClickPos = -1;
    }
    applyEditorChange(change);
    event->setAccepted(true);
}

void TextArea::mouseMoveEvent(MouseEvent* event) {
    moveCursor(positionAtMouse(event->x(), event->y()), true);
    event->setAccepted(true);
}

void TextArea::wheelEvent(MouseEvent* event) {
    if (event->wheelDeltaY() == 0) return;
    if ((focusPolicy() & (Qt::ClickFocus | Qt::WheelFocus)) && !focus()) setFocus(true);
    setContentY(m_contentY - event->wheelDeltaY());
    event->setAccepted(true);
}

int TextArea::positionAtMouse(int x, int y) const {
    const int line = m_contentY + (std::max)(0, y - topPadding());
    const int column = m_contentX + (std::max)(0, x - leftPadding());
    return positionForLineColumn(line, column);
}

void TextArea::ensureGraphemeBoundaries() {
    if (m_graphemeBoundariesValid) return;

    m_graphemeBoundaries.clear();
    const QString& text = m_editor->text();
    int lineOffset = 0;
    for (const auto lineView : QStringTokenizer{text, u'\n'}) {
        GraphemeIterator iterator(lineView, /*sanitizeControls=*/true);
        GraphemeRef cluster;
        while (iterator.next(cluster)) {
            m_graphemeBoundaries.push_back(lineOffset + cluster.utf16Start);
        }
        const int lineEndOffset = lineOffset + static_cast<int>(lineView.size());
        if (lineEndOffset < text.size()) {
            m_graphemeBoundaries.push_back(lineEndOffset);
        }
        lineOffset = lineEndOffset + 1;
    }
    m_graphemeBoundaries.push_back(static_cast<int>(text.size()));
    m_graphemeBoundariesValid = true;
}

void TextArea::snapRangeOutward(int start, int length, int& outStart, int& outEnd) {
    ensureGraphemeBoundaries();
    if (m_graphemeBoundaries.empty()) {
        outStart = start;
        outEnd = start + length;
        return;
    }

    auto itStart =
        std::upper_bound(m_graphemeBoundaries.begin(), m_graphemeBoundaries.end(), start);
    outStart = (itStart != m_graphemeBoundaries.begin()) ? *(itStart - 1) : 0;

    auto itEnd =
        std::lower_bound(m_graphemeBoundaries.begin(), m_graphemeBoundaries.end(), start + length);
    outEnd =
        (itEnd != m_graphemeBoundaries.end()) ? *itEnd : static_cast<int>(m_editor->text().size());
}

bool TextArea::setHighlightLayer(const QString& name, const QList<HighlightRange>& ranges,
                                 quint64 revision, int priority) {
    if (name.isEmpty() || revision != m_textRevision) return false;

    const int textSize = static_cast<int>(m_editor->text().size());
    for (const HighlightRange& r : ranges) {
        const bool valid = (r.start() >= 0) && (r.length() >= 0) && (r.start() <= textSize) &&
                           (r.length() <= textSize - r.start());
        if (!valid) return false;
    }

    QList<HighlightRange> processed;
    processed.reserve(ranges.size());
    for (const HighlightRange& r : ranges) {
        if (r.length() == 0) continue;
        int outStart = 0;
        int outEnd = 0;
        snapRangeOutward(r.start(), r.length(), outStart, outEnd);
        // "transparent" is the absence of a colour: drop it so lower layers or
        // the item's own colour show through, and the tag never reaches a cell.
        TextStyle style = r.style();
        if (style.color().isTransparent()) style.resetColor();
        if (style.backgroundColor().isTransparent()) style.resetBackgroundColor();
        processed.append(HighlightRange(outStart, outEnd - outStart, style, r.link()));
    }

    auto it = m_highlightLayers.find(name);
    if (it != m_highlightLayers.end() && it->priority == priority && it->ranges == processed) {
        return true;
    }

    HighlightLayer layer;
    layer.priority = priority;
    layer.ranges = std::move(processed);
    m_highlightLayers.insert(name, std::move(layer));

    recomposeHighlightRuns();
    emit highlightsChanged();
    markDirty();
    return true;
}

void TextArea::clearHighlightLayer(const QString& name) {
    if (m_highlightLayers.remove(name) > 0) {
        recomposeHighlightRuns();
        emit highlightsChanged();
        markDirty();
    }
}

void TextArea::clearHighlights() {
    if (!m_highlightLayers.isEmpty()) {
        m_highlightLayers.clear();
        m_composedRuns.clear();
        emit highlightsChanged();
        markDirty();
    }
}

void TextArea::recomposeHighlightRuns() {
    m_composedRuns.clear();
    if (m_highlightLayers.isEmpty()) return;

    struct RangeKey {
        int priority = 0;
        QString layerName;
        int rangeIndex = 0;
        int id = 0;

        bool operator<(const RangeKey& other) const {
            if (priority != other.priority) return priority < other.priority;
            if (layerName != other.layerName) return layerName < other.layerName;
            if (rangeIndex != other.rangeIndex) return rangeIndex < other.rangeIndex;
            return id < other.id;
        }
    };

    struct ActiveColor {
        RangeKey key;
        Color color;
        bool operator<(const ActiveColor& other) const { return key < other.key; }
    };

    struct ActiveBool {
        RangeKey key;
        bool value = false;
        bool operator<(const ActiveBool& other) const { return key < other.key; }
    };

    struct ActiveLink {
        RangeKey key;
        QString link;
        bool operator<(const ActiveLink& other) const { return key < other.key; }
    };

    struct Event {
        int position = 0;
        bool isStart = false;
        RangeKey key;
        TextStyle style;
        QString link;
    };

    std::vector<Event> events;
    int nextId = 0;
    for (auto it = m_highlightLayers.cbegin(); it != m_highlightLayers.cend(); ++it) {
        const QString& layerName = it.key();
        const HighlightLayer& layer = it.value();
        for (int rIdx = 0; rIdx < layer.ranges.size(); ++rIdx) {
            const HighlightRange& r = layer.ranges[rIdx];
            if (r.length() <= 0) continue;
            RangeKey key{layer.priority, layerName, rIdx, nextId++};
            events.push_back(Event{r.start(), true, key, r.style(), r.link()});
            events.push_back(Event{r.start() + r.length(), false, key, r.style(), r.link()});
        }
    }

    if (events.empty()) return;

    std::sort(events.begin(), events.end(), [](const Event& a, const Event& b) {
        if (a.position != b.position) return a.position < b.position;
        return !a.isStart && b.isStart;
    });

    std::set<ActiveColor> activeFg;
    std::set<ActiveColor> activeBg;
    std::set<ActiveBool> activeBold;
    std::set<ActiveBool> activeDim;
    std::set<ActiveBool> activeItalic;
    std::set<ActiveBool> activeUnderline;
    std::set<ActiveBool> activeReversed;
    std::set<ActiveBool> activeStrikethrough;
    std::set<ActiveLink> activeLink;

    auto resolveCurrentStyle = [&]() -> TextStyle {
        TextStyle s;
        if (!activeFg.empty()) s.setColor(activeFg.rbegin()->color);
        if (!activeBg.empty()) s.setBackgroundColor(activeBg.rbegin()->color);
        if (!activeBold.empty()) s.setBold(activeBold.rbegin()->value);
        if (!activeDim.empty()) s.setDim(activeDim.rbegin()->value);
        if (!activeItalic.empty()) s.setItalic(activeItalic.rbegin()->value);
        if (!activeUnderline.empty()) s.setUnderline(activeUnderline.rbegin()->value);
        if (!activeReversed.empty()) s.setReversed(activeReversed.rbegin()->value);
        if (!activeStrikethrough.empty()) s.setStrikethrough(activeStrikethrough.rbegin()->value);
        return s;
    };

    int currentPos = events.front().position;
    TextStyle currentStyle;
    QString currentLink;

    size_t i = 0;
    while (i < events.size()) {
        const int pos = events[i].position;
        if (pos > currentPos) {
            if (!currentStyle.isEmpty() || !currentLink.isEmpty()) {
                if (!m_composedRuns.empty() && m_composedRuns.back().end == currentPos &&
                    m_composedRuns.back().style == currentStyle &&
                    m_composedRuns.back().link == currentLink) {
                    m_composedRuns.back().end = pos;
                } else {
                    m_composedRuns.push_back(
                        ComposedRun{currentPos, pos, currentStyle, currentLink});
                }
            }
            currentPos = pos;
        }

        while (i < events.size() && events[i].position == pos) {
            const Event& ev = events[i];
            if (ev.isStart) {
                if (ev.style.color().isValid())
                    activeFg.insert(ActiveColor{ev.key, ev.style.color()});
                if (ev.style.backgroundColor().isValid())
                    activeBg.insert(ActiveColor{ev.key, ev.style.backgroundColor()});
                if (ev.style.hasBold()) activeBold.insert(ActiveBool{ev.key, ev.style.bold()});
                if (ev.style.hasDim()) activeDim.insert(ActiveBool{ev.key, ev.style.dim()});
                if (ev.style.hasItalic())
                    activeItalic.insert(ActiveBool{ev.key, ev.style.italic()});
                if (ev.style.hasUnderline())
                    activeUnderline.insert(ActiveBool{ev.key, ev.style.underline()});
                if (ev.style.hasReversed())
                    activeReversed.insert(ActiveBool{ev.key, ev.style.reversed()});
                if (ev.style.hasStrikethrough())
                    activeStrikethrough.insert(ActiveBool{ev.key, ev.style.strikethrough()});
                if (!ev.link.isEmpty()) activeLink.insert(ActiveLink{ev.key, ev.link});
            } else {
                if (ev.style.color().isValid())
                    activeFg.erase(ActiveColor{ev.key, ev.style.color()});
                if (ev.style.backgroundColor().isValid())
                    activeBg.erase(ActiveColor{ev.key, ev.style.backgroundColor()});
                if (ev.style.hasBold()) activeBold.erase(ActiveBool{ev.key, ev.style.bold()});
                if (ev.style.hasDim()) activeDim.erase(ActiveBool{ev.key, ev.style.dim()});
                if (ev.style.hasItalic()) activeItalic.erase(ActiveBool{ev.key, ev.style.italic()});
                if (ev.style.hasUnderline())
                    activeUnderline.erase(ActiveBool{ev.key, ev.style.underline()});
                if (ev.style.hasReversed())
                    activeReversed.erase(ActiveBool{ev.key, ev.style.reversed()});
                if (ev.style.hasStrikethrough())
                    activeStrikethrough.erase(ActiveBool{ev.key, ev.style.strikethrough()});
                if (!ev.link.isEmpty()) activeLink.erase(ActiveLink{ev.key, ev.link});
            }
            ++i;
        }

        currentStyle = resolveCurrentStyle();
        currentLink = activeLink.empty() ? QString() : activeLink.rbegin()->link;
    }
}

void TextArea::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!visible()) return;
    const int viewportWidth = (std::max)(0, width() - leftPadding() - rightPadding());
    const int viewportHeight = (std::max)(0, height() - topPadding() - bottomPadding());
    const ClipRect contentClip = clipRect.intersected(
        ClipRect{absX + leftPadding(), absY + topPadding(), viewportWidth, viewportHeight});

    const bool isPlaceholder = m_editor->text().isEmpty() && !activeFocus();
    const uint32_t displayColor = m_resolvedColor;
    const TextAttribute displayAttributes =
        !isEffectivelyEnabled() || isPlaceholder ? TextAttribute::Dim : TextAttribute::None;

    const bool useRuns = !isPlaceholder && !m_composedRuns.empty();
    auto runIt = m_composedRuns.cend();
    bool runSeeked = false;

    auto paintLine = [&](int visibleLine, int globalOffset, QStringView lineView) {
        if (useRuns && !runSeeked) {
            // First visible line: skip every run that ends before it.
            runIt = std::lower_bound(
                m_composedRuns.cbegin(), m_composedRuns.cend(), globalOffset,
                [](const ComposedRun& run, int offset) { return run.end <= offset; });
            runSeeked = true;
        }

        const int y = absY + topPadding() + visibleLine;
        int displayColumn = 0;
        bool leadingWhitespace = m_showWhitespace && !isPlaceholder;
        GraphemeIterator iterator(lineView, /*sanitizeControls=*/true);
        GraphemeRef cluster;
        while (iterator.next(cluster)) {
            const bool isLeadingSpace = leadingWhitespace && cluster.utf16 == u" ";
            if (cluster.utf16 != u" " && cluster.utf16 != u"\t") {
                leadingWhitespace = false;
            }
            const int x = absX + leftPadding() + displayColumn - m_contentX;
            const int width = (std::max)(1, cluster.width);
            if (x >= absX + leftPadding() && x + width <= absX + leftPadding() + viewportWidth &&
                contentClip.contains(x, y) && contentClip.contains(x + width - 1, y)) {
                const int clusterStart = globalOffset + cluster.utf16Start;
                const int clusterEnd = clusterStart + cluster.utf16Length();

                uint32_t fg = displayColor;
                uint32_t bg = 0;
                TextAttribute attrs = displayAttributes;
                uint8_t linkId = 0;

                if (!isPlaceholder && runIt != m_composedRuns.cend()) {
                    while (runIt != m_composedRuns.cend() && runIt->end <= clusterStart)
                        ++runIt;
                    if (runIt != m_composedRuns.cend() && runIt->start < clusterEnd) {
                        if (runIt->style.color().isValid()) fg = runIt->style.color().argb32();
                        if (runIt->style.backgroundColor().isValid())
                            bg = runIt->style.backgroundColor().argb32();
                        attrs = runIt->style.resolveAttributes(TextAttribute::None);
                        if (!isEffectivelyEnabled()) attrs = attrs | TextAttribute::Dim;
                        if (!runIt->link.isEmpty()) linkId = buffer.registerLink(runIt->link);
                    }
                }

                const QStringView graphemeView = isLeadingSpace ? QStringView(u"·") : cluster.utf16;
                const TextAttribute cellAttrs =
                    isLeadingSpace ? (attrs | TextAttribute::Dim) : attrs;
                buffer.setActiveLinkId(linkId);
                buffer.setGrapheme(x, y, graphemeView, cluster.width, fg, bg, cellAttrs,
                                   (std::max)(absX + leftPadding(), contentClip.x));
                buffer.setActiveLinkId(0);

                if (!m_editor->text().isEmpty() && clusterEnd > selectionStart() &&
                    clusterStart < selectionEnd()) {
                    for (int dx = 0; dx < width; ++dx) {
                        Cell* selectionCell = buffer.tryCellAt(x + dx, y);
                        if (!selectionCell) continue;
                        detail::paintSelectionCell(
                            *selectionCell, m_hasResolvedSelectionColor, m_resolvedSelectionColor,
                            m_hasResolvedSelectedTextColor, m_resolvedSelectedTextColor);
                    }
                }
            }
            displayColumn += cluster.width;
        }
    };

    if (isPlaceholder) {
        int currentLine = 0;
        int globalOffset = 0;
        for (const auto lineView : QStringTokenizer{m_placeholderText, u'\n'}) {
            if (currentLine >= m_contentY) {
                const int visibleLine = currentLine - m_contentY;
                if (visibleLine >= viewportHeight) break;
                paintLine(visibleLine, globalOffset, lineView);
            }
            globalOffset += lineView.size() + 1;
            ++currentLine;
        }
    } else if (viewportHeight > 0) {
        const int lineCount = static_cast<int>(m_lineOffsets.size());
        const int firstLine = std::clamp(m_contentY, 0, lineCount);
        const int endLine = (std::min)(lineCount, firstLine + viewportHeight);
        for (int currentLine = firstLine; currentLine < endLine; ++currentLine) {
            paintLine(currentLine - m_contentY, m_lineOffsets[currentLine],
                      lineViewAt(currentLine));
        }
    }

    if (activeFocus()) {
        int line = 0, column = 0;
        lineColumnForPosition(m_editor->cursorPosition(), line, column);
        const int x = absX + leftPadding() + column - m_contentX;
        const int y = absY + topPadding() + line - m_contentY;
        if (contentClip.contains(x, y)) {
            if (!m_readOnly) {
                if (Terminal* term = terminal()) {
                    if (TerminalEngine* eng = term->engine()) {
                        eng->setHardwareCursorPosition(x, y);
                    }
                }
            }
            const bool appActive = !terminal() || terminal()->applicationActive();
            if (m_cursorVisible && (m_blinkState || !appActive)) {
                if (Cell* cell = buffer.tryCellAt(x, y)) {
                    detail::paintTextCursor(
                        *cell, static_cast<detail::TextCursorShape>(m_cursorShape),
                        m_hasResolvedCursorColor ? std::optional<uint32_t>(m_resolvedCursorColor)
                                                 : std::nullopt);
                }
            }
        }
    }
}

} // namespace qmltui
