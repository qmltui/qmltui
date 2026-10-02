#include "ToolTip.h"
#include "../src/items/Item.h"
#include "../src/items/Terminal.h"
#include "../src/items/Popup.h"
#include "../src/items/Text.h"
#include "../src/core/UnicodeText.h"
#include <algorithm>

namespace qtui::controls {

ToolTip::ToolTip(QObject *target) : QObject(target), m_target(qobject_cast<Item*>(target)) {
    m_delayTimer.setSingleShot(true);
    m_timeoutTimer.setSingleShot(true);
    connect(&m_delayTimer, &QTimer::timeout, this, &ToolTip::showNow);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &ToolTip::hide);
    if (m_target) {
        connect(m_target, &Item::activeFocusChanged, this, &ToolTip::updateAutomaticVisibility);
        connect(m_target, &Item::visibleChanged, this, &ToolTip::updateAutomaticVisibility);
        connect(m_target, &Item::parentChanged, this, &ToolTip::bindTerminal);
    }
    bindTerminal();
}

ToolTip::~ToolTip() {
    if (m_terminal && !m_text.isEmpty())
        m_terminal->setMouseMotionRequested(this, false);
    if (m_popup) {
        m_popup->close();
        delete m_popup.data();
    }
}

void ToolTip::bindTerminal() {
    Terminal *terminal = m_target ? m_target->terminal() : nullptr;
    if (m_terminal == terminal) return;
    if (m_terminal && !m_text.isEmpty())
        m_terminal->setMouseMotionRequested(this, false);
    if (m_popup) {
        m_popup->close();
        delete m_popup.data();
        m_popup.clear();
        m_label.clear();
    }
    if (m_terminal) disconnect(m_terminal, nullptr, this, nullptr);
    m_terminal = terminal;
    if (m_terminal) {
        connect(m_terminal, &Terminal::hoveredItemChanged, this, &ToolTip::updateAutomaticVisibility);
        connect(m_terminal, &Terminal::focusedItemChanged, this, &ToolTip::updateAutomaticVisibility);
        if (!m_text.isEmpty())
            m_terminal->setMouseMotionRequested(this, true);
    }
    updateAutomaticVisibility();
}

void ToolTip::setText(const QString &text) {
    if (m_text == text) return;
    const bool requestedMotion = !m_text.isEmpty();
    m_text = text;
    if (m_terminal && requestedMotion != !m_text.isEmpty())
        m_terminal->setMouseMotionRequested(this, !m_text.isEmpty());
    emit textChanged();
    if (m_text.isEmpty()) hide(); else updateAutomaticVisibility();
}

bool ToolTip::visible() const { return m_popup && m_popup->visible(); }

void ToolTip::setVisible(bool visible) {
    m_explicitVisible = visible;
    visible ? showNow() : hide();
}

void ToolTip::setDelay(int delay) {
    delay = std::max(0, delay);
    if (m_delay == delay) return;
    m_delay = delay;
    emit delayChanged();
}

void ToolTip::setTimeout(int timeout) {
    timeout = std::max(0, timeout);
    if (m_timeout == timeout) return;
    m_timeout = timeout;
    emit timeoutChanged();
}

void ToolTip::setZ(qreal z) {
    if (qFuzzyCompare(m_z, z)) return;
    m_z = z;
    if (m_popup) m_popup->setZ(z);
    emit zChanged();
}

bool ToolTip::targetIsActive() const {
    if (!m_target || !m_terminal || !m_target->visible()) return false;
    if (m_target->activeFocus()) return true;
    for (Item *item = m_terminal->hoveredItem(); item; item = item->parentItem())
        if (item == m_target) return true;
    return false;
}

void ToolTip::updateAutomaticVisibility() {
    if (m_explicitVisible) { showNow(); return; }
    if (!targetIsActive() || m_text.isEmpty()) { hide(); return; }
    if (visible() || m_delayTimer.isActive()) return;
    m_delayTimer.start(m_delay);
}

void ToolTip::showNow() {
    m_delayTimer.stop();
    if (!m_target || !m_terminal || m_text.isEmpty()) return;
    const bool wasVisible = visible();
    if (!m_popup) {
        m_popup = new Popup(m_terminal);
        m_popup->setOwner(m_target);
        m_popup->setModal(false);
        m_popup->setDimBackground(false);
        m_popup->setClosePolicy(Popup::NoAutoClose);
        m_popup->setColor(QStringLiteral("#303030"));
        m_label = new Text(m_popup);
        m_label->setX(1);
        m_label->setY(0);
    }
    const int maxTextWidth = std::max(1, std::min(40, m_terminal->width() / 2));
    const int displayWidth = UnicodeText::displayWidth(m_text);
    const int textWidth = std::max(1, std::min(maxTextWidth, displayWidth));
    const int lines = std::max(1, (displayWidth + textWidth - 1) / textWidth);
    m_label->setText(m_text);
    m_label->setWidth(textWidth);
    m_label->setHeight(lines);
    m_popup->setWidth(textWidth + 2);
    m_popup->setHeight(lines);
    m_popup->setZ(m_z);
    const QPoint targetPos = m_target->absolutePosition();
    int x = std::clamp(targetPos.x(), 0, std::max(0, m_terminal->width() - m_popup->width()));
    int y = targetPos.y() + m_target->height();
    if (y + m_popup->height() > m_terminal->height()) y = targetPos.y() - m_popup->height();
    m_popup->setX(x);
    m_popup->setY(std::max(0, y));
    if (!wasVisible) {
        m_popup->open();
        emit visibleChanged();
    }
    if (m_timeout > 0) m_timeoutTimer.start(m_timeout);
}

void ToolTip::hide() {
    m_delayTimer.stop();
    m_timeoutTimer.stop();
    if (!visible()) return;
    m_popup->close();
    emit visibleChanged();
}

} // namespace qtui::controls
