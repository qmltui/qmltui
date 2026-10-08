#include <Qtui/Controls/TabBar.h>
#include <Qtui/Core/primitives/ColorMapper.h>
#include <Qtui/Core/engine/KeyEvent.h>
#include <Qtui/Core/engine/MouseEvent.h>
#include <algorithm>

namespace qtui {
void TabBar::setSelectionColor(Color color) {
    if (m_selectionColor == color) return;
    m_selectionColor = color;
    emit selectionColorChanged();
    markDirty();
}

void TabBar::setSelectedTextColor(Color color) {
    if (m_selectedTextColor == color) return;
    m_selectedTextColor = color;
    emit selectedTextColorChanged();
    markDirty();
}

void TabBar::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    emit colorChanged();
    markDirty();
}

void TabBar::setTextColor(Color color) {
    if (m_textColor == color) return;
    m_textColor = color;
    emit textColorChanged();
    markDirty();
}


TabBar::TabBar(QObject *parent) : Item(parent) {
    setFocusable(true);
    setImplicitWidth(30);
    setImplicitHeight(1);

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
    // An index chosen before the tabs arrived (QML initialises properties in
    // declaration order) takes effect now; a list that shrank clamps it.
    if (!m_tabList.isEmpty()) {
        const int clamped = std::clamp(m_currentIndex, 0, static_cast<int>(m_tabList.size()) - 1);
        if (clamped != m_currentIndex) {
            m_currentIndex = clamped;
            emit currentIndexChanged();
        }
    }
    markDirty();
}

void TabBar::setCurrentIndex(int index) {
    // Without tabs the index is kept as requested (never negative) and
    // clamped once setTabs() knows the range.
    const int clamped = m_tabList.isEmpty()
        ? std::max(0, index)
        : std::clamp(index, 0, static_cast<int>(m_tabList.size()) - 1);
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

    const uint32_t inactiveFg = ColorMapper::fromString(m_textColor, 0);
    const uint32_t background = ColorMapper::fromString(m_color, 0);
    const uint32_t activeFg = ColorMapper::fromString(m_selectedTextColor, inactiveFg);
    const uint32_t activeBg = ColorMapper::fromString(m_selectionColor, background);
    const bool reverseSelection = !m_selectedTextColor.isValid() && !m_selectionColor.isValid();

    int curX = absX;
    for (int i = 0; i < m_tabList.size(); ++i) {
        const bool isActive = (i == m_currentIndex);
        const QString tabText = m_tabList[i];
        const QString formatted = isActive ? QString("[ %1 ]").arg(tabText) : QString("  %1  ").arg(tabText);

        if (curX + formatted.length() > absX + availW) break;

        buffer.writeText(curX, absY, formatted,
                         isActive ? activeFg : inactiveFg,
                         isActive ? activeBg : background,
                         (isActive ? TextAttribute::Bold : TextAttribute::None)
                             | (isActive && reverseSelection ? TextAttribute::Reversed : TextAttribute::None)
                             | (!isEffectivelyEnabled() ? TextAttribute::Dim : TextAttribute::None));
        curX += formatted.length();

        if (i < m_tabList.size() - 1 && curX < absX + availW) {
            buffer.writeText(curX, absY, "│", inactiveFg, background);
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
