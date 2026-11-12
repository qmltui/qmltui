#include <QmlTui/Controls/TabBar.h>
#include <QmlTui/Core/Primitives/UnicodeText.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <algorithm>

namespace qmltui {
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

TabBar::TabBar(QObject* parent) : Item(parent) {
    setItemFlag(ItemFlag::BoundedSelfPaint);
    setFocusPolicy(Qt::StrongFocus);
    setImplicitWidth(0);
    setImplicitHeight(1);
    connect(this, &Item::activeFocusChanged, this, [this]() { markDirty(); });
}

void TabBar::setTabs(const QStringList& tabs) {
    if (m_tabList == tabs) return;
    m_tabList = tabs;

    int totalW = 0;
    for (int i = 0; i < m_tabList.size(); ++i) {
        totalW += UnicodeText::displayWidth(m_tabList[i], true) + 4;
        if (i + 1 < m_tabList.size()) totalW += 1;
    }
    setImplicitWidth(totalW);

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
                            ? (std::max)(0, index)
                            : std::clamp(index, 0, static_cast<int>(m_tabList.size()) - 1);
    if (m_currentIndex == clamped) return;
    m_currentIndex = clamped;
    emit currentIndexChanged();
    markDirty();
}

void TabBar::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!visible() || m_tabList.isEmpty()) return;

    const int availW = width();
    const int availH = height();
    if (availW <= 0 || availH <= 0) return;

    const ClipRect effectiveClip = clipRect.intersected(ClipRect{absX, absY, availW, availH});
    if (effectiveClip.isEmpty() || absY < effectiveClip.y ||
        absY >= effectiveClip.y + effectiveClip.height) {
        return;
    }

    const uint32_t inactiveFg =
        Color::paintedArgb32(Color::resolve(m_textColor, Color::DefaultTag));
    const uint32_t background = Color::paintedArgb32(Color::resolve(m_color, Color::DefaultTag));
    const uint32_t activeFg = Color::paintedArgb32(Color::resolve(m_selectedTextColor, inactiveFg));
    const uint32_t activeBg = Color::paintedArgb32(Color::resolve(m_selectionColor, background));
    const bool reverseSelection = !m_selectedTextColor.isValid() && !m_selectionColor.isValid();
    const TextAttribute disabledAttr =
        !isEffectivelyEnabled() ? TextAttribute::Dim : TextAttribute::None;
    const TextAttribute focusAttr = activeFocus() ? TextAttribute::Underline : TextAttribute::None;

    auto writeClipped = [&](int startX, QStringView str, uint32_t fg, uint32_t bg,
                            TextAttribute attrs) {
        int gx = startX;
        GraphemeIterator it(str, true);
        GraphemeRef cluster;
        while (it.next(cluster)) {
            const int cw = cluster.width;
            if (cw == 0) {
                if (gx > effectiveClip.x && gx <= effectiveClip.x + effectiveClip.width) {
                    buffer.setGrapheme(gx, absY, cluster.utf16, 0, fg, bg, attrs, effectiveClip.x);
                }
                continue;
            }
            if (gx >= effectiveClip.x && gx + cw <= effectiveClip.x + effectiveClip.width) {
                buffer.setGrapheme(gx, absY, cluster.utf16, cw, fg, bg, attrs, effectiveClip.x);
            }
            gx += cw;
        }
    };

    int curX = absX;
    for (int i = 0; i < m_tabList.size(); ++i) {
        const bool isActive = (i == m_currentIndex);
        const QString tabText = m_tabList[i];
        const QString formatted =
            isActive ? QString("[ %1 ]").arg(tabText) : QString("  %1  ").arg(tabText);
        const int formattedW = UnicodeText::displayWidth(formatted, true);

        if (curX + formattedW > absX + availW) break;

        const TextAttribute attrs =
            (isActive ? (TextAttribute::Bold | focusAttr |
                         (reverseSelection ? TextAttribute::Reversed : TextAttribute::None))
                      : TextAttribute::None) |
            disabledAttr;

        writeClipped(curX, formatted, isActive ? activeFg : inactiveFg,
                     isActive ? activeBg : background, attrs);
        curX += formattedW;

        if (i < m_tabList.size() - 1 && curX < absX + availW) {
            writeClipped(curX, QStringLiteral("│"), inactiveFg, background, disabledAttr);
            curX += 1;
        }
    }
}

void TabBar::keyPressEvent(KeyEvent* event) {
    if (!isEffectivelyEnabled()) return;
    if (event->key() == Qt::Key_Left) {
        setCurrentIndex(m_currentIndex - 1);
        event->setAccepted(true);
    } else if (event->key() == Qt::Key_Right) {
        setCurrentIndex(m_currentIndex + 1);
        event->setAccepted(true);
    }
}

void TabBar::mousePressEvent(MouseEvent* event) {
    if (!isEffectivelyEnabled()) return;
    const int clickX = event->x();
    if (clickX < 0 || clickX >= width() || event->y() < 0 || event->y() >= height()) return;
    setFocus(true);
    int curX = 0;
    for (int i = 0; i < m_tabList.size(); ++i) {
        const int labelW = UnicodeText::displayWidth(m_tabList[i], true) + 4;
        if (curX + labelW > width()) break;
        if (clickX >= curX && clickX < curX + labelW) {
            setCurrentIndex(i);
            event->setAccepted(true);
            return;
        }
        curX += labelW + 1;
    }
}

} // namespace qmltui
