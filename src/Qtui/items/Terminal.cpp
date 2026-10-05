#include "Terminal.h"
#include "../engine/TerminalEngine.h"
#include "../engine/FrameRenderer.h"
#include "../engine/StreamRenderer.h"
#include "Popup.h"
#include <algorithm>
#include <utility>
#include <cstring>

namespace qtui {

static Terminal::TestRunnerHook s_testRunnerHook = nullptr;

void Terminal::registerTestRunnerHook(TestRunnerHook hook) {
    s_testRunnerHook = hook;
}

void Terminal::componentComplete() {
    if (s_testRunnerHook) {
        if (QQmlEngine* engine = qmlEngine(this)) {
            s_testRunnerHook(engine, this);
        }
    }
}

Terminal::Terminal(QObject *parent) : Rectangle(parent) {
    m_engine = new TerminalEngine(this, this);
    connect(this, &Item::dirty, m_engine, &TerminalEngine::requestRender);

}

Terminal::~Terminal() {
    delete m_engine;
    for (const QMetaObject::Connection& connection : std::as_const(m_mouseMotionRequesters))
        disconnect(connection);
    m_mouseMotionRequesters.clear();
}

void Terminal::print(const QString& text) {
    if (m_engine) {
        m_engine->print(text);
    }
}

void Terminal::append(qtui::Item* item) {
    if (m_engine) {
        m_engine->append(item);
    }
}

void Terminal::copyToClipboard(const QString& text) {
    emit clipboardCopyRequested(text);
    if (m_engine && m_clipboardMode == ClipboardMode::Osc52)
        m_engine->copyToClipboard(text);
}

void Terminal::setTitle(const QString& title) {
    if (m_title == title) return;
    m_title = title;
    emit titleChanged();
    if (m_engine) {
        m_engine->setTitle(title);
    }
}

void Terminal::setUseAlternateScreen(bool use) {
    if (m_useAlternateScreen == use) return;
    m_useAlternateScreen = use;
    emit useAlternateScreenChanged();
}

void Terminal::registerPopup(Popup* popup) {
    if (!popup) return;
    m_popups.removeOne(popup);
    popup->setOpenSerial(++m_popupSerial);
    m_popups.append(popup);
    markDirty();
}

void Terminal::unregisterPopup(Popup* popup) {
    if (!popup || !m_popups.contains(popup)) return;
    const QList<Popup*> ordered = orderedPopups();
    for (auto it = ordered.crbegin(); it != ordered.crend(); ++it)
        if (*it && (*it)->visible() && (*it)->isDescendantOfPopup(popup)) (*it)->close();
    m_popups.removeOne(popup);
    markDirty();
}

QList<Popup*> Terminal::orderedPopups() const {
    QList<Popup*> result = m_popups;
    std::stable_sort(result.begin(), result.end(), [](const Popup *a, const Popup *b) {
        if (!a || !b) return a != nullptr;
        if (!qFuzzyCompare(a->z(), b->z())) return a->z() < b->z();
        return a->openSerial() < b->openSerial();
    });
    Popup *modalBarrier = nullptr;
    for (auto it = result.crbegin(); it != result.crend(); ++it) {
        if (*it && (*it)->visible() && (*it)->modal()) { modalBarrier = *it; break; }
    }
    if (modalBarrier) {
        QList<Popup*> below;
        QList<Popup*> group;
        for (Popup *popup : result) {
            if (popup == modalBarrier || (popup && popup->isDescendantOfPopup(modalBarrier)))
                group.append(popup);
            else
                below.append(popup);
        }
        below.append(group);
        result = below;
    }
    return result;
}

Popup *Terminal::topModalPopup() const {
    const QList<Popup*> ordered = orderedPopups();
    for (auto it = ordered.crbegin(); it != ordered.crend(); ++it)
        if (*it && (*it)->visible() && (*it)->modal()) return *it;
    return nullptr;
}

QList<InputScope*> Terminal::orderedInputScopes() const {
    const QList<Popup*> popups = orderedPopups();
    QList<InputScope*> scopes;
    scopes.reserve(popups.size());
    for (Popup* p : popups) {
        if (p) scopes.append(p);
    }
    return scopes;
}

InputScope* Terminal::topModalScope() const {
    return topModalPopup();
}

void Terminal::setFocusedItem(Item* item) {
    if (item && (!item->focusable() || !item->visible() || !item->isEffectivelyEnabled()))
        item = nullptr;
    if (m_focusedItem == item) return;
    
    Item* oldFocus = m_focusedItem;
    m_focusedItem = item;
    
    if (oldFocus) {
        oldFocus->setFocus(false);
        oldFocus->setActiveFocus(false);
    }
    if (m_focusedItem) {
        m_focusedItem->setActiveFocus(true);
    }
    emit focusedItemChanged();
}

void Terminal::setHoveredItem(Item *item) {
    if (m_hoveredItem == item) return;
    m_hoveredItem = item;
    emit hoveredItemChanged();
}

void Terminal::setMouseMotionRequested(QObject* requester, bool requested) {
    if (!requester) return;

    const bool wasRequested = hasMouseMotionRequests();
    if (requested) {
        if (m_mouseMotionRequesters.contains(requester)) return;
        const QMetaObject::Connection connection = connect(
            requester, &QObject::destroyed, this, [this, requester] {
                if (m_mouseMotionRequesters.remove(requester) > 0 &&
                    m_mouseMotionRequesters.isEmpty()) {
                    emit mouseMotionRequestedChanged(false);
                }
            });
        m_mouseMotionRequesters.insert(requester, connection);
    } else {
        const auto it = m_mouseMotionRequesters.find(requester);
        if (it == m_mouseMotionRequesters.end()) return;
        disconnect(it.value());
        m_mouseMotionRequesters.erase(it);
    }

    const bool isRequested = hasMouseMotionRequests();
    if (wasRequested != isRequested)
        emit mouseMotionRequestedChanged(isRequested);
}

void Terminal::findFocusableItems(Item* root, QList<Item*>& list) {
    if (!root || !root->visible() || !root->isEffectivelyEnabled()) return;
    
    if (root->focusable()) {
        list.append(root);
    }
    
    for (Item* child : root->childItems()) {
        findFocusableItems(child, list);
    }
}

void Terminal::setFillTerminal(bool fill) {
    m_fillTerminalExplicit = true;
    if (m_fillTerminal == fill) return;
    m_fillTerminal = fill;
    emit fillTerminalChanged();
    emit widthChanged();
    emit heightChanged();
    markDirty();
}

void Terminal::setWidth(int w) {
    if (m_fillTerminal && !m_fillTerminalExplicit) {
        m_fillTerminal = false;
        emit fillTerminalChanged();
    }
    Item::setWidth(w);
}

void Terminal::setHeight(int h) {
    if (m_fillTerminal && !m_fillTerminalExplicit) {
        m_fillTerminal = false;
        emit fillTerminalChanged();
    }
    Item::setHeight(h);
}

void Terminal::setRendererMode(RendererMode mode) {
    if (m_rendererMode == mode) return;
    m_rendererMode = mode;
    
    if (!m_mouseReportingExplicit) {
        bool autoReporting = (m_rendererMode == RendererMode::Frame);
        if (m_mouseReporting != autoReporting) {
            m_mouseReporting = autoReporting;
            emit mouseReportingChanged();
            if (m_engine) {
                m_engine->setMouseReporting(m_mouseReporting);
            }
        }
    }

    if (m_engine) {
        if (m_rendererMode == RendererMode::Frame) {
            m_engine->setRenderer(new FrameRenderer());
        } else {
            m_engine->setRenderer(new StreamRenderer());
        }
    }
    
    emit rendererModeChanged();
    markDirty();
}

void Terminal::setClipboardMode(ClipboardMode mode) {
    if (m_clipboardMode == mode) return;
    m_clipboardMode = mode;
    emit clipboardModeChanged();
}

void Terminal::setMouseReporting(bool enable) {
    m_mouseReportingExplicit = true;
    if (m_mouseReporting == enable) return;
    m_mouseReporting = enable;
    emit mouseReportingChanged();
    if (m_engine) {
        m_engine->setMouseReporting(enable);
    }
}

int Terminal::width() const {
    if (m_engine) {
        int w = Item::width();
        return w > 0 ? w : m_engine->terminalWidth();
    }
    return Item::width();
}

void Terminal::setLiveItem(Item* item) {
    if (m_liveItem == item) return;
    if (m_liveItem) {
        disconnect(m_liveItem, &Item::heightChanged, this, &Item::implicitHeightChanged);
        disconnect(m_liveItem, &Item::implicitHeightChanged, this, &Item::implicitHeightChanged);
        disconnect(m_liveItem, &Item::visibleChanged, this, &Item::implicitHeightChanged);
    }
    m_liveItem = item;
    if (m_liveItem) {
        if (m_liveItem->parentItem() != this && !m_liveItem->parentItem()) {
            m_liveItem->setParentItem(this);
        }
        connect(m_liveItem, &Item::heightChanged, this, &Item::implicitHeightChanged);
        connect(m_liveItem, &Item::implicitHeightChanged, this, &Item::implicitHeightChanged);
        connect(m_liveItem, &Item::visibleChanged, this, &Item::implicitHeightChanged);
    }
    emit liveItemChanged();
    emit implicitHeightChanged();
    markDirty();
}

int Terminal::implicitHeight() const {
    if (m_implicitHeight > 0)
        return m_implicitHeight;

    int maxBottom = 0;
    if (m_liveItem && m_liveItem->visible()) {
        int liveH = m_liveItem->height() > 0 ? m_liveItem->height() : m_liveItem->implicitHeight();
        maxBottom = std::max(maxBottom, m_liveItem->y() + liveH);
    }

    for (Item* child : childItems()) {
        if (!child || !child->visible()) continue;
        int childH = child->height() > 0 ? child->height() : child->implicitHeight();
        maxBottom = std::max(maxBottom, child->y() + childH);
    }
    return maxBottom;
}

int Terminal::height() const {
    if (m_rendererMode == RendererMode::Stream) {
        int h = implicitHeight();
        if (h <= 0) h = Item::height();
        return h > 0 ? h : (m_engine ? m_engine->terminalHeight() : 0);
    }
    if (m_fillTerminal && m_engine) {
        return m_engine->terminalHeight();
    }
    return Item::height();
}

int Terminal::terminalWidth() const {
    return m_engine ? m_engine->terminalWidth() : width();
}

int Terminal::terminalHeight() const {
    return m_engine ? m_engine->terminalHeight() : (m_rendererMode == RendererMode::Stream ? 24 : height());
}

static void dimBuffer(TextGridBuffer& buffer, const ClipRect& clipRect) {
    for (int y = 0; y < buffer.height(); ++y) {
        for (int x = 0; x < buffer.width(); ++x) {
            if (clipRect.contains(x, y)) {
                Cell& cell = buffer.cellAt(x, y);
                // Dim fg (shift right by 1 to halve, or divide by 2)
                uint8_t a_fg = (cell.fgColor >> 24) & 0xFF;
                uint8_t r_fg = (cell.fgColor >> 16) & 0xFF;
                uint8_t g_fg = (cell.fgColor >> 8) & 0xFF;
                uint8_t b_fg = cell.fgColor & 0xFF;
                cell.fgColor = (a_fg << 24) | ((r_fg / 2) << 16) | ((g_fg / 2) << 8) | (b_fg / 2);
                
                // Dim bg
                uint8_t a_bg = (cell.bgColor >> 24) & 0xFF;
                uint8_t r_bg = (cell.bgColor >> 16) & 0xFF;
                uint8_t g_bg = (cell.bgColor >> 8) & 0xFF;
                uint8_t b_bg = cell.bgColor & 0xFF;
                cell.bgColor = (a_bg << 24) | ((r_bg / 2) << 16) | ((g_bg / 2) << 8) | (b_bg / 2);
            }
        }
    }
}

void Terminal::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;

    Rectangle::paint(buffer, absX, absY, clipRect);
}

void Terminal::paintOverlay(TextGridBuffer &buffer, int absX, int absY,
                            const ClipRect &clipRect)
{
    Q_UNUSED(absX);
    Q_UNUSED(absY);
    for (Popup* popup : orderedPopups()) {
        if (popup && popup->visible()) {
            if (popup->modal() && popup->dimBackground()) {
                dimBuffer(buffer, clipRect);
            }
            
            QPoint absPos = popup->absolutePosition();
            // Pass the absolute terminal bounds as clipRect (clipRect is usually full screen)
            popup->render(buffer, absPos.x(), absPos.y(), clipRect);
        }
    }
}

} // namespace qtui
