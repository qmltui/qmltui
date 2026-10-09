#include <Qtui/Core/items/Terminal.h>
#include <Qtui/Core/engine/TerminalEngine.h>
#include <Qtui/Core/engine/FrameRenderer.h>
#include <Qtui/Core/engine/StreamRenderer.h>
#include <Qtui/Core/items/Popup.h>
#include <Qtui/Core/primitives/ColorMapper.h>
#include <Qtui/Core/engine/ItemPaintTraversal_p.h>
#include <algorithm>
#include <utility>
#include <cstring>

namespace qtui {

static Terminal::RunnerHook s_runnerHook = nullptr;

void Terminal::registerRunnerHook(RunnerHook hook) {
    s_runnerHook = hook;
}

void Terminal::componentComplete() {
    initializeEngine();
    if (!m_engine) return;
    // Children complete before the root; honour any `focus: true` that was set
    // while the tree was still being assembled.
    applyPendingFocus();
    if (s_runnerHook) {
        if (QQmlEngine* engine = qmlEngine(this)) {
            s_runnerHook(engine, this);
        }
    }
}

Terminal::Terminal(QObject *parent) : Item(parent) {
    setItemFlag(ItemFlag::IsTerminalRoot);
    // QML parentage and mode are not established in the constructor. C++
    // applications can initialize synchronously through engine().
    QMetaObject::invokeMethod(this, &Terminal::initializeEngine, Qt::QueuedConnection);
    connect(this, &Item::implicitHeightChanged, this, [this] {
        if (m_mode == Mode::Inline) emit heightChanged();
    });
}

TerminalEngine* Terminal::engine() const {
    const_cast<Terminal*>(this)->initializeEngine();
    return m_engine;
}

TerminalCapabilities Terminal::capabilities() const {
    return m_engine ? m_engine->capabilities() : TerminalCapabilities{};
}

bool Terminal::enqueueControlSequence(QByteArray bytes) {
    auto* outputEngine = engine();
    return outputEngine && outputEngine->enqueueControlSequence(std::move(bytes));
}

QByteArray Terminal::takeControlSequences() {
    return m_engine ? m_engine->takeControlSequences() : QByteArray{};
}

bool Terminal::enqueuePostControlSequence(QByteArray bytes) {
    auto* outputEngine = engine();
    return outputEngine && outputEngine->enqueuePostControlSequence(std::move(bytes));
}

QByteArray Terminal::takePostControlSequences() {
    return m_engine ? m_engine->takePostControlSequences() : QByteArray{};
}

void Terminal::initializeEngine() {
    if (m_engine) return;
    for (QObject* ancestor = parent(); ancestor; ancestor = ancestor->parent()) {
        if (qobject_cast<Terminal*>(ancestor)) {
            qWarning("Terminal must be a root; use ordinary Items for pages.");
            return;
        }
    }
    m_engine = new TerminalEngine(this, this);
    connect(m_engine, &TerminalEngine::capabilitiesChanged, this, &Terminal::capabilitiesChanged);
    if (m_mode == Mode::Inline) m_engine->setRenderer(new StreamRenderer());
    connect(this, &Item::dirty, m_engine, &TerminalEngine::requestRender);
    emit capabilitiesChanged();
    emit terminalSizeChanged();
    emit widthChanged();
    emit heightChanged();
}

Terminal::~Terminal() {
    delete m_engine;
    for (const QMetaObject::Connection& connection : std::as_const(m_mouseMotionRequesters))
        disconnect(connection);
    m_mouseMotionRequesters.clear();
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

void Terminal::setColor(Color color) {
    if (m_color == color) return;
    m_color = color;
    m_background = ColorMapper::fromString(color, 0x00000000);
    emit colorChanged();
    markDirty();
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

void Terminal::setWidth(int) {
    qWarning("Terminal width is managed by the session; size an inner Item instead.");
}

void Terminal::setHeight(int) {
    qWarning("Terminal height is managed by mode; size an inner Item instead.");
}

void Terminal::setMode(Mode mode) {
    if (m_mode == mode) return;
    m_mode = mode;
    
    if (!m_mouseReportingExplicit) {
        bool autoReporting = (m_mode == Mode::FullScreen);
        if (m_mouseReporting != autoReporting) {
            m_mouseReporting = autoReporting;
            emit mouseReportingChanged();
            if (m_engine) {
                m_engine->setMouseReporting(m_mouseReporting);
            }
        }
    }

    if (m_engine) {
        if (m_mode == Mode::FullScreen) {
            m_engine->setRenderer(new FrameRenderer());
        } else {
            m_engine->setRenderer(new StreamRenderer());
        }
    }
    
    emit modeChanged();
    emit heightChanged();
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
    return terminalWidth();
}

int Terminal::implicitHeight() const {
    if (m_implicitHeight > 0)
        return m_implicitHeight;

    int maxBottom = 0;

    for (Item* child : childItems()) {
        if (!child || !child->visible()) continue;
        int childH = child->height() > 0 ? child->height() : child->implicitHeight();
        maxBottom = std::max(maxBottom, child->y() + childH);
    }
    return maxBottom;
}

int Terminal::height() const {
    if (m_mode == Mode::Inline)
        return std::clamp(implicitHeight(), 0, std::max(0, terminalHeight() - 1));
    return terminalHeight();
}

int Terminal::terminalWidth() const {
    return m_engine ? m_engine->terminalWidth() : 80;
}

int Terminal::terminalHeight() const {
    return m_engine ? m_engine->terminalHeight() : 24;
}

static void dimBuffer(TextGridBuffer& buffer, const ClipRect& clipRect) {
    for (int y = 0; y < buffer.height(); ++y) {
        for (int x = 0; x < buffer.width(); ++x) {
            if (clipRect.contains(x, y)) {
                Cell& cell = buffer.cellAt(x, y);
                cell.attributes = cell.attributes | TextAttribute::Dim;
            }
        }
    }
}

void Terminal::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    if (!m_visible) return;

    if ((m_background >> 24) == 0) return;
    const int right = std::min(buffer.width(), absX + width());
    const int bottom = std::min(buffer.height(), absY + height());
    for (int y = std::max(0, absY); y < bottom; ++y) {
        for (int x = std::max(0, absX); x < right; ++x) {
            if (!clipRect.contains(x, y)) continue;
            Cell& cell = buffer.cellAt(x, y);
            cell.bgColor = m_background;
            cell.setCharacter(" ");
            cell.displayWidth = 1;
            cell.continuation = false;
        }
    }
}

void Terminal::paintOverlay(TextGridBuffer &buffer, int absX, int absY,
                            const ClipRect &clipRect) const {
    Q_UNUSED(absX);
    Q_UNUSED(absY);
    for (Popup* popup : orderedPopups()) {
        if (popup && popup->visible()) {
            if (popup->modal() && popup->dimBackground()) {
                dimBuffer(buffer, clipRect);
            }
            
            QPoint absPos = popup->absolutePosition();
            // Pass the absolute terminal bounds as clipRect (clipRect is usually full screen)
            ItemPaintTraversal::paintSubtree(popup, buffer, absPos.x(),
                                             absPos.y(), clipRect);
        }
    }
}

} // namespace qtui
