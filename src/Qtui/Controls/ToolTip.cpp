#include <Qtui/Controls/ToolTip.h>
#include <Qtui/Core/engine/Clock_p.h>
#include <Qtui/Core/items/Item.h>
#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/items/Popup.h>
#include <Qtui/Core/items/Text.h>
#include <Qtui/Core/primitives/UnicodeText.h>
#include <algorithm>

namespace qtui::controls {
void ToolTip::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    emit colorChanged();
    updateAppearance();
}

void ToolTip::setTextColor(Color color) {
    if (m_textColor == color) return;
    m_textColor = color;
    emit textColorChanged();
    updateAppearance();
}


ToolTip::ToolTip(QObject *target) : QObject(target), m_target(qobject_cast<Item*>(target)) {
    m_delayTimer = new ClockTimer(this);
    m_timeoutTimer = new ClockTimer(this);
    m_delayTimer->setSingleShot(true);
    m_timeoutTimer->setSingleShot(true);
    connect(m_delayTimer, &ClockTimer::timeout, this, &ToolTip::showNow);
    connect(m_timeoutTimer, &ClockTimer::timeout, this, &ToolTip::hide);
    if (m_target) {
        connect(m_target, &Item::activeFocusChanged, this, &ToolTip::updateAutomaticVisibility);
        connect(m_target, &Item::visibleChanged, this, &ToolTip::updateAutomaticVisibility);
        connect(m_target, &Item::parentChanged, this, &ToolTip::bindTerminal);
        // Reaches the target when an ancestor is reparented (a Repeater
        // inserting its delegate), which parentChanged does not report.
        connect(m_target, &Item::ancestryChanged, this, &ToolTip::bindTerminal);
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
    // The ancestor chain may change without the terminal changing (an item
    // moved between containers), so re-track it before the early return.
    trackAncestryGeometry();
    Terminal *terminal = m_target ? m_target->terminal() : nullptr;
    if (m_terminal == terminal) { updatePosition(); return; }
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
        connect(m_terminal, &Terminal::terminalSizeChanged, this, &ToolTip::updatePosition);
        if (!m_text.isEmpty())
            m_terminal->setMouseMotionRequested(this, true);
    }
    updateAutomaticVisibility();
}

// A visible tip must follow its target: the target's own geometry and the
// position of every ancestor decide the absolute anchor, and a tip pinned
// with `visible: true` opens before Row/Column have placed the target at all.
void ToolTip::trackAncestryGeometry() {
    for (const auto &connection : std::as_const(m_geometryConnections))
        disconnect(connection);
    m_geometryConnections.clear();
    if (!m_target) return;
    m_geometryConnections.append(connect(m_target, &Item::widthChanged, this, &ToolTip::updatePosition));
    m_geometryConnections.append(connect(m_target, &Item::heightChanged, this, &ToolTip::updatePosition));
    for (Item *item = m_target; item; item = item->parentItem()) {
        m_geometryConnections.append(connect(item, &Item::xChanged, this, &ToolTip::updatePosition));
        m_geometryConnections.append(connect(item, &Item::yChanged, this, &ToolTip::updatePosition));
    }
}

void ToolTip::updatePosition() {
    if (visible()) reposition();
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
    if (visible() || m_delayTimer->isActive()) return;
    m_delayTimer->start(m_delay);
}

void ToolTip::showNow() {
    m_delayTimer->stop();
    if (!m_target || !m_terminal || m_text.isEmpty()) return;
    const bool wasVisible = visible();
    if (!m_popup) {
        m_popup = new Popup(m_terminal);
        m_popup->setOwner(m_target);
        m_popup->setModal(false);
        m_popup->setDimBackground(false);
        m_popup->setClosePolicy(Popup::NoAutoClose);
        m_label = new Text(m_popup);
        m_label->setBold(true);
        m_label->setX(1);
        m_label->setY(0);
    }
    updateAppearance();
    reposition();
    if (!wasVisible) {
        m_popup->open();
        emit visibleChanged();
    }
    if (m_timeout > 0) m_timeoutTimer->start(m_timeout);
}

void ToolTip::reposition() {
    if (!m_target || !m_terminal || !m_popup || !m_label) return;
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
}

void ToolTip::updateAppearance() {
    if (!m_target || !m_popup || !m_label) return;
    m_popup->setColor(m_color);
    m_label->setColor(m_textColor);
}

void ToolTip::hide() {
    m_delayTimer->stop();
    m_timeoutTimer->stop();
    if (!visible()) return;
    m_popup->close();
    emit visibleChanged();
}

} // namespace qtui::controls
