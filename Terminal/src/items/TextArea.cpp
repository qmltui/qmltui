#include "TextArea.h"
#include "../core/ColorMapper.h"
#include "../engine/KeyEvent.h"
#include <QStringList>

namespace qtui {

TextArea::TextArea(QObject* parent) : Item(parent) {
    setFocusable(true);
}

TextArea::~TextArea() = default;

void TextArea::setText(const QString& text) {
    if (m_text == text) return;
    m_text = text;
    if (m_cursorPosition > m_text.length()) m_cursorPosition = m_text.length();
    
    int newLines = m_text.count('\n') + 1;
    setImplicitHeight(newLines);
    
    int maxWidth = 0;
    for (const QString& line : m_text.split('\n')) {
        if (line.length() > maxWidth) maxWidth = line.length();
    }
    setImplicitWidth(maxWidth + 1);

    emit textChanged();
    markDirty();
}

void TextArea::setColor(const QString& color) {
    if (m_color == color) return;
    m_color = color;
    m_resolvedColor = ColorMapper::fromString(m_color, 0xFFFFFFFF);
    emit colorChanged();
    markDirty();
}

void TextArea::setCursorPosition(int pos) {
    if (pos < 0) pos = 0;
    if (pos > m_text.length()) pos = m_text.length();
    if (m_cursorPosition == pos) return;
    m_cursorPosition = pos;
    emit cursorPositionChanged();
    markDirty();
}

void TextArea::getCursorLineCol(int& line, int& col) const {
    line = 0;
    col = 0;
    for (int i = 0; i < m_cursorPosition && i < m_text.length(); ++i) {
        if (m_text[i] == '\n') {
            line++;
            col = 0;
        } else {
            col++;
        }
    }
}

int TextArea::getPositionFromLineCol(int line, int col) const {
    int currentLine = 0;
    int currentCol = 0;
    for (int i = 0; i < m_text.length(); ++i) {
        if (currentLine == line && currentCol == col) return i;
        if (m_text[i] == '\n') {
            if (currentLine == line) return i; // Cap at end of line
            currentLine++;
            currentCol = 0;
        } else {
            currentCol++;
        }
    }
    return m_text.length();
}

void TextArea::keyPressEvent(KeyEvent* event) {
    if (!activeFocus()) return;

    int key = event->key();
    QString text = event->text();

    if (key == Qt::Key_Left) {
        setCursorPosition(m_cursorPosition - 1);
        event->setAccepted(true);
    } else if (key == Qt::Key_Right) {
        setCursorPosition(m_cursorPosition + 1);
        event->setAccepted(true);
    } else if (key == Qt::Key_Up) {
        int line, col;
        getCursorLineCol(line, col);
        if (line > 0) {
            setCursorPosition(getPositionFromLineCol(line - 1, col));
        } else {
            setCursorPosition(0);
        }
        event->setAccepted(true);
    } else if (key == Qt::Key_Down) {
        int line, col;
        getCursorLineCol(line, col);
        setCursorPosition(getPositionFromLineCol(line + 1, col));
        event->setAccepted(true);
    } else if (key == Qt::Key_Backspace) {
        if (m_cursorPosition > 0) {
            QString newText = m_text;
            newText.remove(m_cursorPosition - 1, 1);
            m_cursorPosition--;
            setText(newText);
        }
        event->setAccepted(true);
    } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        QString newText = m_text;
        newText.insert(m_cursorPosition, '\n');
        m_cursorPosition++;
        setText(newText);
        event->setAccepted(true);
    } else if (!text.isEmpty()) {
        QString newText = m_text;
        newText.insert(m_cursorPosition, text);
        m_cursorPosition += text.length();
        setText(newText);
        event->setAccepted(true);
    }
}

void TextArea::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    Item::render(buffer, absX, absY, clipRect);

    int w = width();
    int h = height();
    int lp = leftPadding();
    int tp = topPadding();
    
    ClipRect contentClip = clipRect;
    if (clip()) {
        contentClip = clipRect.intersected(ClipRect{absX, absY, w, h});
    }

    QStringList lines = m_text.split('\n');
    for (int y = 0; y < lines.size(); ++y) {
        int cy = absY + tp + y;
        if (cy < absY || cy >= absY + h) continue;

        const QString& line = lines[y];
        for (int x = 0; x < line.length(); ++x) {
            int cx = absX + lp + x;
            if (cx < absX || cx >= absX + w) continue;
            
            if (contentClip.contains(cx, cy)) {
                if (cx >= 0 && cx < buffer.width() && cy >= 0 && cy < buffer.height()) {
                    Cell& cell = buffer.cellAt(cx, cy);
                    QByteArray utf8 = line.mid(x, 1).toUtf8();
                    int len = qMin(utf8.size(), 3);
                    for (int i = 0; i < len; ++i) {
                        cell.character[i] = utf8[i];
                    }
                    cell.character[len] = '\0';
                    cell.fgColor = m_resolvedColor;
                }
            }
        }
    }

    if (activeFocus()) {
        int line, col;
        getCursorLineCol(line, col);
        int cx = absX + lp + col;
        int cy = absY + tp + line;
        
        if (cx >= absX && cx < absX + w && cy >= absY && cy < absY + h) {
            if (contentClip.contains(cx, cy)) {
                if (cx >= 0 && cx < buffer.width() && cy >= 0 && cy < buffer.height()) {
                    Cell& cell = buffer.cellAt(cx, cy);
                    cell.attributes = static_cast<TextAttribute>(
                        static_cast<uint8_t>(cell.attributes) | static_cast<uint8_t>(TextAttribute::Reversed)
                    );
                }
            }
        }
    }
}

} // namespace qtui
