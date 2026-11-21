#include <QmlTui/Controls/ToolTip.h>
#include <QmlTui/Core/Engine/Clock_p.h>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/Popup.h>
#include <QmlTui/Core/Items/Rectangle.h>
#include <QmlTui/Core/Items/Text.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>
#include <algorithm>

namespace qmltui {
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

Border* ToolTip::border() const {
    return m_defaultBackground ? m_defaultBackground->border() : nullptr;
}

void ToolTip::setBackground(Item* background) {
    if (m_background == background) return;
    if (m_background && m_popup && m_background->parentItem() == m_popup) {
        m_background->setVisible(false);
        m_background->setParentItem(nullptr);
        if (m_background == m_defaultBackground) m_background->setParent(this);
    }
    m_background = background;
    trackBackgroundSignals();
    syncPopupChildren();
    updateAppearance();
    updatePosition();
    emit backgroundChanged();
}

void ToolTip::setContentItem(Item* contentItem) {
    if (m_contentItem == contentItem) return;
    const bool hadMotion = hasContent();
    if (m_contentItem && m_popup && m_contentItem->parentItem() == m_popup) {
        m_contentItem->setVisible(false);
        m_contentItem->setParentItem(nullptr);
        if (m_contentItem == m_defaultLabel) m_contentItem->setParent(this);
    }
    m_contentItem = contentItem;
    trackContentSignals();
    if (m_terminal && hadMotion != hasContent())
        m_terminal->setMouseMotionRequested(this, hasContent());
    syncPopupChildren();
    updateAppearance();
    emit contentItemChanged();
    if (!hasContent()) {
        hide();
    } else {
        updateAutomaticVisibility();
        updatePosition();
    }
}

ToolTip::ToolTip(QObject* target) : QObject(target), m_target(qobject_cast<Item*>(target)) {
    m_delayTimer = new ClockTimer(this);
    m_timeoutTimer = new ClockTimer(this);
    m_delayTimer->setSingleShot(true);
    m_timeoutTimer->setSingleShot(true);
    connect(m_delayTimer, &ClockTimer::timeout, this, &ToolTip::showNow);
    connect(m_timeoutTimer, &ClockTimer::timeout, this, [this]() {
        m_explicitVisible = false;
        hide();
    });

    m_defaultBackground = new Rectangle(this);
    m_defaultBackground->setVisible(false);
    m_defaultLabel = new Text(this);
    m_defaultLabel->setBold(true);
    m_defaultLabel->setWrapMode(Text::WordWrap);
    m_defaultLabel->setVisible(false);
    m_background = m_defaultBackground;
    m_contentItem = m_defaultLabel;
    trackBackgroundSignals();
    trackContentSignals();

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
    for (const auto& conn : std::as_const(m_backgroundConnections))
        disconnect(conn);
    for (const auto& conn : std::as_const(m_contentConnections))
        disconnect(conn);
    if (m_terminal && hasContent()) m_terminal->setMouseMotionRequested(this, false);
    if (m_popup) {
        m_popup->close();
        delete m_popup.data();
    }
}

void ToolTip::trackBackgroundSignals() {
    for (const auto& conn : std::as_const(m_backgroundConnections))
        disconnect(conn);
    m_backgroundConnections.clear();
    if (!m_background) return;
    m_backgroundConnections.append(
        connect(m_background, &Item::implicitWidthChanged, this, &ToolTip::updatePosition));
    m_backgroundConnections.append(
        connect(m_background, &Item::implicitHeightChanged, this, &ToolTip::updatePosition));
    if (auto* rect = qobject_cast<Rectangle*>(m_background.data())) {
        m_backgroundConnections.append(
            connect(rect->border(), &Border::styleChanged, this, [this]() {
                updateAppearance();
                updatePosition();
            }));
        m_backgroundConnections.append(
            connect(rect->border(), &Border::colorChanged, this, &ToolTip::updateAppearance));
    }
}

void ToolTip::trackContentSignals() {
    for (const auto& conn : std::as_const(m_contentConnections))
        disconnect(conn);
    m_contentConnections.clear();
    if (!m_contentItem || m_contentItem == m_defaultLabel) return;
    m_contentConnections.append(
        connect(m_contentItem, &Item::implicitWidthChanged, this, &ToolTip::updatePosition));
    m_contentConnections.append(
        connect(m_contentItem, &Item::implicitHeightChanged, this, &ToolTip::updatePosition));
    m_contentConnections.append(
        connect(m_contentItem, &Item::widthChanged, this, &ToolTip::updatePosition));
    m_contentConnections.append(
        connect(m_contentItem, &Item::heightChanged, this, &ToolTip::updatePosition));
}

void ToolTip::syncPopupChildren() {
    if (!m_popup) return;
    if (m_defaultBackground && m_background != m_defaultBackground &&
        m_defaultBackground->parentItem() == m_popup) {
        m_defaultBackground->setVisible(false);
        m_defaultBackground->setParentItem(nullptr);
        m_defaultBackground->setParent(this);
    }
    if (m_defaultLabel && m_contentItem != m_defaultLabel &&
        m_defaultLabel->parentItem() == m_popup) {
        m_defaultLabel->setVisible(false);
        m_defaultLabel->setParentItem(nullptr);
        m_defaultLabel->setParent(this);
    }
    if (m_background) {
        m_background->setParentItem(m_popup);
        m_background->setVisible(true);
        m_background->setZ(-1);
    }
    if (m_contentItem) {
        m_contentItem->setParentItem(m_popup);
        m_contentItem->setVisible(true);
        m_contentItem->setZ(0);
    }
}

void ToolTip::detachPopupChildren() {
    if (!m_popup) return;
    m_popup->disconnect();
    if (m_background && m_background->parentItem() == m_popup) {
        m_background->setVisible(false);
        m_background->setParentItem(nullptr);
        m_background->setParent(this);
    }
    if (m_contentItem && m_contentItem->parentItem() == m_popup) {
        m_contentItem->setVisible(false);
        m_contentItem->setParentItem(nullptr);
        m_contentItem->setParent(this);
    }
}

void ToolTip::bindTerminal() {
    // The ancestor chain may change without the terminal changing (an item
    // moved between containers), so re-track it before the early return.
    trackAncestryGeometry();
    Terminal* terminal = m_target ? m_target->terminal() : nullptr;
    if (m_terminal == terminal) {
        updatePosition();
        return;
    }
    if (m_terminal && hasContent()) m_terminal->setMouseMotionRequested(this, false);
    if (m_popup) {
        m_popup->close();
        detachPopupChildren();
        delete m_popup.data();
        m_popup.clear();
    }
    if (m_terminal) disconnect(m_terminal, nullptr, this, nullptr);
    m_terminal = terminal;
    if (m_terminal) {
        connect(m_terminal, &Terminal::hoveredItemChanged, this,
                &ToolTip::updateAutomaticVisibility);
        connect(m_terminal, &Terminal::focusedItemChanged, this,
                &ToolTip::updateAutomaticVisibility);
        connect(m_terminal, &Terminal::terminalSizeChanged, this, &ToolTip::updatePosition);
        if (hasContent()) m_terminal->setMouseMotionRequested(this, true);
    }
    updateAutomaticVisibility();
}

// A visible tip must follow its target: the target's own geometry and the
// position of every ancestor decide the absolute anchor, and a tip pinned
// with `visible: true` opens before Row/Column have placed the target at all.
void ToolTip::trackAncestryGeometry() {
    for (const auto& connection : std::as_const(m_geometryConnections))
        disconnect(connection);
    m_geometryConnections.clear();
    if (!m_target) return;
    m_geometryConnections.append(
        connect(m_target, &Item::widthChanged, this, &ToolTip::updatePosition));
    m_geometryConnections.append(
        connect(m_target, &Item::heightChanged, this, &ToolTip::updatePosition));
    for (Item* item = m_target; item; item = item->parentItem()) {
        m_geometryConnections.append(
            connect(item, &Item::xChanged, this, &ToolTip::updatePosition));
        m_geometryConnections.append(
            connect(item, &Item::yChanged, this, &ToolTip::updatePosition));
    }
}

void ToolTip::updatePosition() {
    if (visible()) reposition();
}

bool ToolTip::hasContent() const {
    return !m_text.isEmpty() || (m_contentItem && m_contentItem != m_defaultLabel);
}

void ToolTip::setText(const QString& text) {
    if (m_text == text) return;
    const bool requestedMotion = hasContent();
    m_text = text;
    if (m_terminal && requestedMotion != hasContent())
        m_terminal->setMouseMotionRequested(this, hasContent());
    emit textChanged();
    if (!hasContent()) {
        hide();
    } else {
        updateAutomaticVisibility();
        updatePosition();
    }
}

bool ToolTip::visible() const {
    return m_popup && m_popup->visible();
}

void ToolTip::setVisible(bool visible) {
    m_explicitVisible = visible;
    visible ? showNow() : hide();
}

void ToolTip::setDelay(int delay) {
    delay = (std::max)(0, delay);
    if (m_delay == delay) return;
    m_delay = delay;
    emit delayChanged();
}

void ToolTip::setTimeout(int timeout) {
    timeout = (std::max)(0, timeout);
    if (m_timeout == timeout) return;
    m_timeout = timeout;
    if (visible()) {
        if (m_timeout > 0)
            m_timeoutTimer->start(m_timeout);
        else
            m_timeoutTimer->stop();
    }
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
    Item* hovered = m_terminal->hoveredItem();
    return hovered && hovered->isDescendantOf(m_target);
}

void ToolTip::updateAutomaticVisibility() {
    if (m_explicitVisible) {
        showNow();
        return;
    }
    if (!targetIsActive() || !hasContent()) {
        hide();
        return;
    }
    if (visible() || m_delayTimer->isActive()) return;
    m_delayTimer->start(m_delay);
}

void ToolTip::showNow() {
    m_delayTimer->stop();
    if (!m_target || !m_terminal || !hasContent()) return;
    const bool wasVisible = visible();
    if (!m_popup) {
        m_popup = new Popup(m_terminal);
        m_popup->setOwner(m_target);
        m_popup->setModal(false);
        m_popup->setDimBackground(false);
        m_popup->setClosePolicy(Popup::NoAutoClose);
    }
    syncPopupChildren();
    updateAppearance();
    reposition();
    if (!wasVisible) {
        m_popup->open();
        emit visibleChanged();
    }
    if (m_timeout > 0) m_timeoutTimer->start(m_timeout);
}

void ToolTip::reposition() {
    if (!m_target || !m_terminal || !m_popup || !m_contentItem) return;
    auto* bgRect = qobject_cast<Rectangle*>(m_background.data());
    const bool hasBorder = bgRect && bgRect->border()->style() != Border::None;
    const int padX = hasBorder ? 2 : 1;
    const int padY = hasBorder ? 1 : 0;

    int contentW = 1;
    int contentH = 1;
    if (m_contentItem == m_defaultLabel && m_defaultLabel) {
        const int maxTextWidth = (std::max)(1, (std::min)(40, m_terminal->width() / 2));
        m_defaultLabel->setText(m_text);
        const int naturalWidth = (std::max)(1, UnicodeText::displayWidth(m_text));
        contentW = (std::max)(1, (std::min)(maxTextWidth, naturalWidth));
        m_defaultLabel->setWidth(contentW);
        contentH = (std::max)(1, m_defaultLabel->implicitHeight());
        m_defaultLabel->setHeight(contentH);
    } else {
        contentW = (std::max)(1, m_contentItem->width() > 0 ? m_contentItem->width()
                                                            : m_contentItem->implicitWidth());
        contentH = (std::max)(1, m_contentItem->height() > 0 ? m_contentItem->height()
                                                             : m_contentItem->implicitHeight());
    }
    m_contentItem->setX(padX);
    m_contentItem->setY(padY);

    const int popupW =
        (std::max)(m_background ? m_background->implicitWidth() : 0, contentW + padX * 2);
    const int popupH =
        (std::max)(m_background ? m_background->implicitHeight() : 0, contentH + padY * 2);
    m_popup->setWidth(popupW);
    m_popup->setHeight(popupH);
    m_popup->setZ(m_z);
    if (m_background) {
        m_background->setX(0);
        m_background->setY(0);
        m_background->setWidth(popupW);
        m_background->setHeight(popupH);
    }
    const QPoint targetPos = m_target->absolutePosition();
    int x = std::clamp(targetPos.x(), 0, (std::max)(0, m_terminal->width() - m_popup->width()));
    int y = targetPos.y() + m_target->height();
    if (y + m_popup->height() > m_terminal->height()) y = targetPos.y() - m_popup->height();
    m_popup->setX(x);
    m_popup->setY((std::max)(0, y));
}

void ToolTip::updateAppearance() {
    if (m_defaultBackground) m_defaultBackground->setColor(m_color);
    if (m_defaultLabel) m_defaultLabel->setColor(m_textColor);
}

void ToolTip::hide() {
    m_delayTimer->stop();
    m_timeoutTimer->stop();
    if (!visible()) return;
    m_popup->close();
    emit visibleChanged();
}

} // namespace qmltui
