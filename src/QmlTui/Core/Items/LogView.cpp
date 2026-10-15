#include <QmlTui/Core/Items/LogView.h>

#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <algorithm>
#include <utility>

namespace qmltui {

LogView::LogView(QObject* parent) : TextArea(parent) {
    setReadOnly(true);
    connect(this, &TextArea::textChanged, this, [this] {
        if (!m_applyingBufferedText) {
            m_lines = text().split('\n');
            m_pendingTrimmedChars = 0;
        }
        if (m_followTail) scrollToTail();
    });
}

void LogView::setMaximumLineCount(int count) {
    count = (std::max)(1, count);
    if (m_maximumLineCount == count) return;
    m_maximumLineCount = count;
    emit maximumLineCountChanged();
    if (m_lines.size() > m_maximumLineCount) {
        trimHistory();
        applyBufferedText();
    }
}

void LogView::setFollowTail(bool follow) {
    if (m_followTail == follow) return;
    m_followTail = follow;
    emit followTailChanged();
    if (follow) scrollToTail();
}

void LogView::appendLine(const QString& line) {
    if (m_lines.size() == 1 && m_lines.first().isEmpty())
        m_lines.first() = line;
    else
        m_lines.append(line);
    trimHistory();
    applyBufferedText();
}

void LogView::appendText(const QString& value) {
    if (value.isEmpty()) return;
    if (m_lines.isEmpty()) m_lines.append(QString());
    const QStringList incoming = value.split('\n');
    m_lines.last().append(incoming.first());
    for (qsizetype index = 1; index < incoming.size(); ++index)
        m_lines.append(incoming[index]);
    trimHistory();
    applyBufferedText();
}

void LogView::clear() {
    m_lines.clear();
    m_pendingTrimmedChars = 0;
    clearHighlights();
    applyBufferedText();
    setCursorPosition(0);
    setContentX(0);
    setContentY(0);
}

void LogView::keyPressEvent(KeyEvent* event) {
    if (event->key() == Qt::Key_End && (event->modifiers() & Qt::ControlModifier)) {
        setFollowTail(true);
        setCursorPosition(text().size());
        event->setAccepted(true);
        return;
    }
    if (event->key() == Qt::Key_Up || event->key() == Qt::Key_PageUp ||
        event->key() == Qt::Key_Home)
        setFollowTail(false);
    TextArea::keyPressEvent(event);
}

void LogView::wheelEvent(MouseEvent* event) {
    if (!event || event->wheelDeltaY() == 0) return;
    if (event->wheelDeltaY() > 0) setFollowTail(false);
    TextArea::wheelEvent(event);
}

void LogView::trimHistory() {
    if (m_lines.size() <= m_maximumLineCount) return;
    const qsizetype removeCount = m_lines.size() - m_maximumLineCount;
    for (qsizetype i = 0; i < removeCount; ++i)
        m_pendingTrimmedChars += static_cast<int>(m_lines[i].size()) + 1;
    m_lines.erase(m_lines.begin(), m_lines.begin() + removeCount);
}

void LogView::scrollToTail() {
    setContentY((std::max)(0, static_cast<int>(m_lines.size()) - height()));
}

void LogView::applyBufferedText() {
    m_applyingBufferedText = true;
    replaceTextPreservingHighlights(m_lines.join('\n'), std::exchange(m_pendingTrimmedChars, 0));
    m_applyingBufferedText = false;
    if (m_followTail) scrollToTail();
}

} // namespace qmltui
