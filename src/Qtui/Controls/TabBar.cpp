#include "TabBar.h"
#include "Palette.h"
#include <ColorMapper.h>
#include <KeyEvent.h>
#include <MouseEvent.h>
#include <algorithm>

namespace qtui {

TabBar::TabBar(QObject *parent) : Item(parent) {
    setFocusable(true);
    setImplicitWidth(30);
    setImplicitHeight(1);
    connect(Palette::instance(), &Palette::changed, this, [this] { markDirty(); });
}

void TabBar::setTabs(const QVariant &tabs) {
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

void TabBar::setCurrentIndex(int index) {
    if (m_tabList.isEmpty()) return;
    int clamped = std::clamp(index, 0, static_cast<int>(m_tabList.size()) - 1);
    if (m_currentIndex == clamped) return;
    m_currentIndex = clamped;
    emit currentIndexChanged();
    markDirty();
}

void TabBar::paint(TextGridBuffer &buffer, int absX, int absY, const ClipRect &clipRect) const {
    Q_UNUSED(clipRect);
    if (!m_visible || m_tabList.isEmpty()) return;

    const int availW = width();
    const int availH = height();
    if (availW <= 0 || availH <= 0) return;

    const Palette* palette = Palette::instance();
    const uint32_t activeFg = ColorMapper::fromString(palette->highlightedText(), 0xFF000000);
    const uint32_t activeBg = ColorMapper::fromString(palette->highlight(), 0xFFFFFF00);
    const uint32_t inactiveFg = ColorMapper::fromString(palette->windowText(), 0xFFFFFFFF);
    const uint32_t separatorFg = ColorMapper::fromString(palette->mid(), 0xFF808080);

    int curX = absX;
    for (int i = 0; i < m_tabList.size(); ++i) {
        const bool isActive = (i == m_currentIndex);
        const QString tabText = m_tabList[i];
        const QString formatted = isActive ? QString("[ %1 ]").arg(tabText) : QString("  %1  ").arg(tabText);

        if (curX + formatted.length() > absX + availW) break;

        buffer.writeText(curX, absY, formatted,
                         isActive ? activeFg : inactiveFg,
                         isActive ? activeBg : 0x00000000,
                         isActive ? TextAttribute::Bold : TextAttribute::None);
        curX += formatted.length();

        if (i < m_tabList.size() - 1 && curX < absX + availW) {
            buffer.writeText(curX, absY, "│", separatorFg, 0x00000000);
            curX += 1;
        }
    }
}

void TabBar::keyPressEvent(KeyEvent *event) {
    if (event->key() == Qt::Key_Left) {
        setCurrentIndex(m_currentIndex - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Right) {
        setCurrentIndex(m_currentIndex + 1);
        event->setAccepted(true);
    }
}

void TabBar::mousePressEvent(MouseEvent *event) {
    setFocus(true);
    const int clickX = event->x();
    int curX = 0;
    for (int i = 0; i < m_tabList.size(); ++i) {
        const int tabW = m_tabList[i].length() + 4 + 1;
        if (clickX >= curX && clickX < curX + tabW) {
            setCurrentIndex(i);
            event->setAccepted(true);
            return;
        }
        curX += tabW;
    }
}

} // namespace qtui
