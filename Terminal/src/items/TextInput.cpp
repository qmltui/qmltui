#include "TextInput.h"
#include "../engine/KeyEvent.h"
#include "../core/ColorMapper.h"
#include "../core/UnicodeText.h"
#include <algorithm>

namespace qtui {

TextInput::TextInput(QObject *parent) : Item(parent) {
    setFocusable(true);
    setImplicitWidth(10);
    setImplicitHeight(1);
}

TextInput::~TextInput() = default;

void TextInput::setText(const QString& text) {
    if (m_text == text) return;
    m_text = text;
    setImplicitWidth(std::max(10, UnicodeText::displayWidth(m_text) + 2));
    if (m_cursorPosition > m_text.length()) {
        m_cursorPosition = m_text.length();
        emit cursorPositionChanged();
    }
    emit textChanged();
    markDirty();
}

void TextInput::setColor(const QString& color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    emit colorChanged();
    markDirty();
}

void TextInput::setCursorPosition(int pos) {
    int clamped = std::clamp(pos, 0, static_cast<int>(m_text.length()));
    if (m_cursorPosition == clamped) return;
    m_cursorPosition = clamped;
    emit cursorPositionChanged();
    markDirty();
}

void TextInput::keyPressEvent(KeyEvent* event) {
    int key = event->key();
    QString keyText = event->text();

    if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
        // Let Tab/Backtab bubble up for focus cycling
        return;
    }

    if (key == Qt::Key_Left) {
        setCursorPosition(UnicodeText::previousBoundary(m_text, m_cursorPosition));
        event->setAccepted(true);
    } else if (key == Qt::Key_Right) {
        setCursorPosition(UnicodeText::nextBoundary(m_text, m_cursorPosition));
        event->setAccepted(true);
    } else if (key == Qt::Key_Backspace) {
        if (m_cursorPosition > 0) {
            const int previous = UnicodeText::previousBoundary(m_text, m_cursorPosition);
            m_text.remove(previous, m_cursorPosition - previous);
            setCursorPosition(previous);
            emit textChanged();
            markDirty();
        }
        event->setAccepted(true);
    } else if (key == Qt::Key_Delete) {
        if (m_cursorPosition < m_text.length()) {
            const int next = UnicodeText::nextBoundary(m_text, m_cursorPosition);
            m_text.remove(m_cursorPosition, next - m_cursorPosition);
            emit textChanged();
            markDirty();
        }
        event->setAccepted(true);
    } else if (key == Qt::Key_Home) {
        setCursorPosition(0);
        event->setAccepted(true);
    } else if (key == Qt::Key_End) {
        setCursorPosition(m_text.length());
        event->setAccepted(true);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        emit accepted();
        event->setAccepted(true);
    } else if (!keyText.isEmpty() && keyText[0].isPrint()) {
        m_text.insert(m_cursorPosition, keyText);
        setCursorPosition(m_cursorPosition + keyText.length());
        emit textChanged();
        markDirty();
        event->setAccepted(true);
    }
}

void TextInput::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;

    Item::render(buffer, absX, absY, clipRect);

    int lp = leftPadding();
    int rp = rightPadding();
    int tp = topPadding();
    int bp = bottomPadding();

    int renderY = absY + tp;

    // Render the text string
    if (renderY >= absY + tp && renderY < absY + height() - bp && renderY >= 0 && renderY < buffer.height()) {
        int cx = absX + lp;
        for (const Grapheme &cluster : UnicodeText::graphemes(m_text)) {
            if (cx >= absX + lp && cx + cluster.width <= absX + width() - rp &&
                clipRect.contains(cx, renderY))
                buffer.writeText(cx, renderY, QString::fromUtf8(cluster.utf8),
                                 isEffectivelyEnabled() ? m_resolvedColor : 0xFF808080);
            cx += cluster.width;
        }
    }

    // Render cursor if focused
    if (activeFocus()) {
        int cx = absX + lp + UnicodeText::displayWidth(m_text.left(m_cursorPosition));
        int cy = absY + tp;
        if (cx >= absX + lp && cx < absX + width() - rp) {
            if (clipRect.contains(cx, cy)) {
                if (cx >= 0 && cx < buffer.width() && cy >= 0 && cy < buffer.height()) {
                    Cell& cell = buffer.cellAt(cx, cy);
                    uint32_t fg = cell.fgColor;
                    uint32_t bg = cell.bgColor;
                    if ((bg >> 24) == 0) bg = 0xFFCCCCCC; // light grey block background
                    cell.fgColor = bg;
                    cell.bgColor = fg != 0 ? fg : 0xFF000000;
                    if (cell.character.isEmpty())
                        cell.character = " ";
                }
            }
        }
    }
}

} // namespace qtui
