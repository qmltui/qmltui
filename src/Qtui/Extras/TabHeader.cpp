#include "TabHeader.h"
#include <ColorMapper.h>
#include <KeyEvent.h>
#include <MouseEvent.h>
#include <algorithm>

namespace qtui {

TabHeader::TabHeader(QObject *parent) : Item(parent) {
    setFocusable(true);
    setImplicitWidth(30);
    setImplicitHeight(1);
}

void TabHeader::setTabs(const QVariant &tabs) {
    m_tabsVariant = tabs;
    m_tabList.clear();

    if (tabs.userType() == QMetaType::fromType<QJSValue>().id()) {
        QVariant v = tabs.value<QJSValue>().toVariant();
        if (v.canConvert<QStringList>()) m_tabList = v.toStringList();
        else if (v.canConvert<QVariantList>()) {
            for (const QVariant &item : v.toList()) m_tabList.append(item.toString());
        }
    } else if (tabs.canConvert<QStringList>()) {
        m_tabList = tabs.toStringList();
    } else if (tabs.canConvert<QVariantList>()) {
        for (const QVariant &item : tabs.toList()) m_tabList.append(item.toString());
    }

    emit tabsChanged();
    markDirty();
}

void TabHeader::setCurrentIndex(int index) {
    if (m_tabList.isEmpty()) return;
    int clamped = std::clamp(index, 0, static_cast<int>(m_tabList.size()) - 1);
    if (m_currentIndex == clamped) return;
    m_currentIndex = clamped;
    emit currentIndexChanged();
    markDirty();
}

void TabHeader::setActiveTabColor(const QString &color) {
    if (m_activeTabColor == color) return;
    m_activeTabColor = color;
    m_resolvedActiveTabColor = ColorMapper::fromString(m_activeTabColor, 0xFFFFFFFF);
    emit activeTabColorChanged();
    markDirty();
}

void TabHeader::setActiveTabBgColor(const QString &color) {
    if (m_activeTabBgColor == color) return;
    m_activeTabBgColor = color;
    m_resolvedActiveTabBgColor = ColorMapper::fromString(m_activeTabBgColor, 0xFF0000FF);
    emit activeTabBgColorChanged();
    markDirty();
}

void TabHeader::setInactiveTabColor(const QString &color) {
    if (m_inactiveTabColor == color) return;
    m_inactiveTabColor = color;
    m_resolvedInactiveTabColor = ColorMapper::fromString(m_inactiveTabColor, 0xFF808080);
    emit inactiveTabColorChanged();
    markDirty();
}

void TabHeader::setInactiveTabBgColor(const QString &color) {
    if (m_inactiveTabBgColor == color) return;
    m_inactiveTabBgColor = color;
    m_resolvedInactiveTabBgColor = ColorMapper::fromString(m_inactiveTabBgColor, 0x00000000);
    emit inactiveTabBgColorChanged();
    markDirty();
}

void TabHeader::setBorderStyle(const QString &style) {
    if (m_borderStyle == style) return;
    m_borderStyle = style;
    emit borderStyleChanged();
    markDirty();
}

void TabHeader::setBorderColor(const QString &color) {
    if (m_borderColor == color) return;
    m_borderColor = color;
    m_resolvedBorderColor = ColorMapper::fromString(m_borderColor, 0xFF808080);
    emit borderColorChanged();
    markDirty();
}

void TabHeader::render(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) {
    if (!m_visible || m_tabList.isEmpty()) return;

    Item::render(buffer, absX, absY, clipRect);

    int availW = width();
    int availH = height();
    int startX = absX;
    int startY = absY;

    if (m_borderStyle != "none") {
        buffer.drawBox(absX, absY, availW, availH, m_resolvedBorderColor, 0x00000000);
        startX += 1;
        startY += 1;
        availW -= 2;
        availH -= 2;
    }

    if (availW <= 0 || availH <= 0) return;

    int curX = startX;
    for (int i = 0; i < m_tabList.size(); ++i) {
        bool isActive = (i == m_currentIndex);
        QString tabText = m_tabList[i];
        QString formatted = isActive ? QString("[ %1 ]").arg(tabText) : QString("  %1  ").arg(tabText);

        if (curX + formatted.length() > startX + availW) break;

        uint32_t fg = isActive ? m_resolvedActiveTabColor : m_resolvedInactiveTabColor;
        uint32_t bg = isActive ? m_resolvedActiveTabBgColor : m_resolvedInactiveTabBgColor;
        TextAttribute attr = isActive ? TextAttribute::Bold : TextAttribute::None;

        buffer.writeText(curX, startY, formatted, fg, bg, attr);
        curX += formatted.length();

        if (i < m_tabList.size() - 1 && curX < startX + availW) {
            buffer.writeText(curX, startY, "│", m_resolvedInactiveTabColor, 0x00000000);
            curX += 1;
        }
    }
}

void TabHeader::keyPressEvent(KeyEvent *event) {
    if (event->key() == Qt::Key_Left) {
        setCurrentIndex(m_currentIndex - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Right) {
        setCurrentIndex(m_currentIndex + 1);
        event->setAccepted(true);
    }
}

void TabHeader::mousePressEvent(MouseEvent *event) {
    setFocus(true);
    int borderOffset = (m_borderStyle != "none") ? 1 : 0;
    int clickX = event->x() - borderOffset;

    int curX = 0;
    for (int i = 0; i < m_tabList.size(); ++i) {
        QString tabText = m_tabList[i];
        int tabW = (i == m_currentIndex ? tabText.length() + 4 : tabText.length() + 4) + 1;

        if (clickX >= curX && clickX < curX + tabW) {
            setCurrentIndex(i);
            event->setAccepted(true);
            return;
        }
        curX += tabW;
    }
}

} // namespace qtui
