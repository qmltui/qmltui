#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/FocusScope.h>
#include <QmlTui/Core/Engine/Clipboard.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Engine/AltScreenRenderer.h>
#include <QmlTui/Core/Engine/InlineRenderer.h>
#include <QmlTui/Core/Items/Popup.h>
#include <QmlTui/Core/Engine/ItemPaintTraversal_p.h>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QQmlInfo>
#include <algorithm>
#include <utility>

namespace qmltui {

struct TerminalPrivate {
    TerminalEngine* engine = nullptr;
    QPointer<Item> focusedItem;
    QPointer<Item> hoveredItem;
    QHash<QObject*, QMetaObject::Connection> mouseMotionRequesters;
    QHash<Popup*, QMetaObject::Connection> popupDestroyConnections;
    QList<Popup*> popups;
    quint64 popupSerial = 0;
    QString title;
    Color color = Color::Transparent;
    uint32_t background = 0;
    Terminal::Mode mode = Terminal::Mode::AltScreen;
    QPointer<Clipboard> clipboard;
    Terminal::ClipboardMode clipboardMode = Terminal::ClipboardMode::Osc52;
    bool mouseReporting = true;
    bool mouseReportingExplicit = false;
    bool applicationActive = true;
    qreal progress = 0.0;
    Terminal::ProgressState progressState = Terminal::ProgressNone;
};

static Terminal::RunnerHook s_runnerHook = nullptr;

void Terminal::registerRunnerHook(RunnerHook hook) {
    s_runnerHook = hook;
}

void Terminal::componentComplete() {
    initializeEngine();
    if (!m_d->engine) return;
    // Children complete before the root; honour any `focus: true` that was set
    // while the tree was still being assembled.
    applyPendingFocus();
    if (s_runnerHook) {
        if (QQmlEngine* engine = qmlEngine(this)) {
            s_runnerHook(engine, this);
        }
    }
}

Terminal::Terminal(QObject* parent) : Item(parent), m_d(std::make_unique<TerminalPrivate>()) {
    setItemFlag(ItemFlag::IsTerminalRoot);
    setItemFlag(ItemFlag::BoundedSelfPaint);
    // QML parentage and mode are not established in the constructor. C++
    // applications can initialize synchronously through engine().
    QMetaObject::invokeMethod(this, &Terminal::initializeEngine, Qt::QueuedConnection);
    connect(this, &Item::implicitHeightChanged, this, [this] {
        if (m_d->mode == Mode::Inline) emit heightChanged();
    });
}

TerminalEngine* Terminal::engine() const {
    const_cast<Terminal*>(this)->initializeEngine();
    return m_d->engine;
}

TerminalEngine* Terminal::engineIfInitialized() const {
    return m_d->engine;
}

TerminalCapabilities Terminal::capabilities() const {
    return m_d->engine ? m_d->engine->capabilities() : TerminalCapabilities{};
}

bool Terminal::enqueueControlSequence(QByteArray bytes) {
    auto* outputEngine = engine();
    return outputEngine && outputEngine->enqueueControlSequence(std::move(bytes));
}

QByteArray Terminal::takeControlSequences() {
    return m_d->engine ? m_d->engine->takeControlSequences() : QByteArray{};
}

bool Terminal::enqueuePostControlSequence(QByteArray bytes) {
    auto* outputEngine = engine();
    return outputEngine && outputEngine->enqueuePostControlSequence(std::move(bytes));
}

QByteArray Terminal::takePostControlSequences() {
    return m_d->engine ? m_d->engine->takePostControlSequences() : QByteArray{};
}

void Terminal::initializeEngine() {
    if (m_d->engine) return;
    for (QObject* ancestor = parent(); ancestor; ancestor = ancestor->parent()) {
        if (qobject_cast<Terminal*>(ancestor)) {
            qmlWarning(this) << "Terminal must be a root; use ordinary Items for pages.";
            return;
        }
    }
    m_d->engine = new TerminalEngine(this, this);
    connect(m_d->engine, &TerminalEngine::capabilitiesChanged, this,
            &Terminal::capabilitiesChanged);
    if (m_d->mode == Mode::Inline) m_d->engine->setRenderer(new InlineRenderer());
    connect(this, &Item::dirty, m_d->engine, &TerminalEngine::requestRender);
    if (auto* qml = qmlEngine(this)) {
        m_d->clipboard = qml->singletonInstance<Clipboard*>("QmlTui", "Clipboard");
        if (m_d->clipboard) m_d->clipboard->addTerminal(this);
    }
    emit capabilitiesChanged();
    emit terminalSizeChanged();
    emit widthChanged();
    emit heightChanged();
}

Terminal::~Terminal() {
    setItemFlag(ItemFlag::IsTerminalRoot, false);
    if (m_d->clipboard) m_d->clipboard->removeTerminal(this);
    TerminalEngine* eng = m_d->engine;
    m_d->engine = nullptr;
    delete eng;
    for (const QMetaObject::Connection& connection : std::as_const(m_d->mouseMotionRequesters))
        disconnect(connection);
    m_d->mouseMotionRequesters.clear();
}

void Terminal::copyToClipboard(const QString& text) {
    emit clipboardCopyRequested(text);
    if (m_d->engine && m_d->clipboardMode == ClipboardMode::Osc52)
        m_d->engine->copyToClipboard(text);
}

void Terminal::bell() {
    if (m_d->engine) {
        m_d->engine->ringBell();
    }
}

void Terminal::queryColors() {
    if (auto* outputEngine = engine()) {
        outputEngine->queryColors();
    }
}

bool Terminal::applicationActive() const {
    return m_d->applicationActive;
}

void Terminal::setApplicationActive(bool active) {
    if (m_d->applicationActive == active) return;
    m_d->applicationActive = active;
    if (m_d->engine) m_d->engine->markFullDamage();
    emit applicationActiveChanged();
    if (m_d->focusedItem) m_d->focusedItem->markDirty();
    markDirty();
}

qreal Terminal::progress() const {
    return m_d->progress;
}

void Terminal::setProgress(qreal progress) {
    const qreal clamped = std::clamp(progress, 0.0, 1.0);
    if (qFuzzyCompare(1.0 + m_d->progress, 1.0 + clamped)) return;
    m_d->progress = clamped;
    emit progressChanged();
    if (m_d->engine) {
        m_d->engine->syncTerminalProgress();
    }
}

Terminal::ProgressState Terminal::progressState() const {
    return m_d->progressState;
}

void Terminal::setProgressState(ProgressState state) {
    if (m_d->progressState == state) return;
    m_d->progressState = state;
    emit progressStateChanged();
    if (m_d->engine) {
        m_d->engine->syncTerminalProgress();
    }
}

QString Terminal::title() const {
    return m_d->title;
}

void Terminal::setTitle(const QString& title) {
    if (m_d->title == title) return;
    m_d->title = title;
    emit titleChanged();
    if (m_d->engine) {
        m_d->engine->setTitle(title);
    }
}

Color Terminal::color() const {
    return m_d->color;
}

void Terminal::setColor(Color color) {
    if (m_d->color == color) return;
    m_d->color = color;
    m_d->background = Color::resolve(color, 0x00000000);
    if (m_d->engine) m_d->engine->markFullDamage();
    emit colorChanged();
    markDirty();
}

void Terminal::registerPopup(Popup* popup) {
    if (!popup) return;
    m_d->popups.removeOne(popup);
    if (!m_d->popupDestroyConnections.contains(popup)) {
        const QMetaObject::Connection conn =
            connect(popup, &QObject::destroyed, this, [this, popup]() {
                m_d->popupDestroyConnections.remove(popup);
                if (m_d->popups.removeOne(popup)) {
                    if (m_d->engine) m_d->engine->markFullDamage();
                    markDirty();
                }
            });
        m_d->popupDestroyConnections.insert(popup, conn);
    }
    popup->setOpenSerial(++m_d->popupSerial);
    m_d->popups.append(popup);
    if (m_d->engine) m_d->engine->markFullDamage();
    markDirty();
}

void Terminal::unregisterPopup(Popup* popup) {
    if (!popup || !m_d->popups.contains(popup)) return;
    QPointer<Terminal> self(this);
    QPointer<Popup> popupGuard(popup);
    const QList<Popup*> orderedRaw = orderedPopups();
    QList<QPointer<Popup>> ordered;
    ordered.reserve(orderedRaw.size());
    for (Popup* p : orderedRaw) {
        ordered.append(p);
    }
    for (auto it = ordered.crbegin(); it != ordered.crend(); ++it) {
        Popup* child = it->data();
        if (child && child != popup && child->visible() && popupGuard &&
            child->isDescendantOfPopup(popupGuard.data())) {
            child->close();
            if (!self) return;
        }
    }
    if (const auto connIt = m_d->popupDestroyConnections.find(popup);
        connIt != m_d->popupDestroyConnections.end()) {
        disconnect(connIt.value());
        m_d->popupDestroyConnections.erase(connIt);
    }
    m_d->popups.removeOne(popup);
    if (m_d->engine) m_d->engine->markFullDamage();
    markDirty();
}

const QList<Popup*>& Terminal::activePopups() const {
    return m_d->popups;
}

QList<Popup*> Terminal::orderedPopups() const {
    QList<Popup*> result = m_d->popups;
    std::stable_sort(result.begin(), result.end(), [](const Popup* a, const Popup* b) {
        if (!a || !b) return a != nullptr;
        if (!qFuzzyCompare(a->z(), b->z())) return a->z() < b->z();
        return a->openSerial() < b->openSerial();
    });
    Popup* modalBarrier = nullptr;
    for (auto it = result.crbegin(); it != result.crend(); ++it) {
        if (*it && (*it)->visible() && (*it)->modal()) {
            modalBarrier = *it;
            break;
        }
    }
    if (modalBarrier) {
        QList<Popup*> below;
        QList<Popup*> group;
        for (Popup* popup : result) {
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

Popup* Terminal::topModalPopup() const {
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

static QList<QPointer<FocusScope>> getScopeChain(Item* item) {
    QList<QPointer<FocusScope>> chain;
    for (Item* p = item ? item->parentItem() : nullptr; p; p = p->parentItem()) {
        if (p->isFocusScope()) {
            chain.append(static_cast<FocusScope*>(p));
        }
    }
    return chain;
}

Item* Terminal::focusedItem() const {
    return m_d->focusedItem.data();
}

void Terminal::setFocusedItem(Item* item) {
    if (item && (!item->isEffectivelyVisible() || !item->isEffectivelyEnabled())) item = nullptr;
    if (m_d->focusedItem == item) return;

    QPointer<Terminal> self(this);
    QPointer<Item> oldFocus = m_d->focusedItem;
    QPointer<Item> targetFocus = item;
    m_d->focusedItem = item;

    const QList<QPointer<FocusScope>> oldScopes = getScopeChain(oldFocus.data());
    const QList<QPointer<FocusScope>> newScopes = getScopeChain(targetFocus.data());
    const bool oldRemainsActiveScope =
        oldFocus && oldFocus->isFocusScope() &&
        newScopes.contains(QPointer<FocusScope>(static_cast<FocusScope*>(oldFocus.data())));

    if (oldFocus && !oldRemainsActiveScope) {
        oldFocus->setActiveFocus(false);
        if (!self) return;
        if (oldFocus && oldScopes.isEmpty()) {
            oldFocus->setFocus(false);
            if (!self) return;
        }
    }

    for (const QPointer<FocusScope>& scope : oldScopes) {
        if (scope && scope.data() != targetFocus.data() && !newScopes.contains(scope)) {
            scope->setActiveFocus(false);
            if (!self) return;
        }
    }

    // Abort if a signal handler already redirected focus to another item.
    if (m_d->focusedItem != targetFocus) return;

    // Update subFocusItem along the scope hierarchy
    for (Item* leaf = targetFocus.data(); leaf;) {
        FocusScope* scope = leaf->enclosingFocusScope();
        if (scope) {
            scope->setSubFocusItem(leaf);
        }
        leaf = scope;
    }

    for (const QPointer<FocusScope>& scope : newScopes) {
        if (scope && !oldScopes.contains(scope)) {
            scope->setActiveFocus(true);
            if (!self || m_d->focusedItem != targetFocus) return;
        }
    }

    if (targetFocus) {
        targetFocus->setActiveFocus(true);
        if (!self || m_d->focusedItem != targetFocus) return;
        if (targetFocus) {
            targetFocus->setFocus(true);
            if (!self) return;
        }
    }
    emit focusedItemChanged();
}

Item* Terminal::hoveredItem() const {
    return m_d->hoveredItem.data();
}

void Terminal::setHoveredItem(Item* item) {
    if (m_d->hoveredItem == item) return;
    m_d->hoveredItem = item;
    emit hoveredItemChanged();
}

void Terminal::setMouseMotionRequested(QObject* requester, bool requested) {
    if (!requester) return;

    const bool wasRequested = hasMouseMotionRequests();
    if (requested) {
        if (m_d->mouseMotionRequesters.contains(requester)) return;
        const QMetaObject::Connection connection =
            connect(requester, &QObject::destroyed, this, [this, requester] {
                if (m_d->mouseMotionRequesters.remove(requester) &&
                    m_d->mouseMotionRequesters.isEmpty()) {
                    emit mouseMotionRequestedChanged(false);
                }
            });
        m_d->mouseMotionRequesters.insert(requester, connection);
    } else {
        const auto it = m_d->mouseMotionRequesters.find(requester);
        if (it == m_d->mouseMotionRequesters.end()) return;
        disconnect(it.value());
        m_d->mouseMotionRequesters.erase(it);
    }

    const bool isRequested = hasMouseMotionRequests();
    if (wasRequested != isRequested) emit mouseMotionRequestedChanged(isRequested);
}

bool Terminal::isMouseMotionRequester(const QObject* requester) const {
    return requester && m_d->mouseMotionRequesters.contains(const_cast<QObject*>(requester));
}

bool Terminal::hasMouseMotionRequests() const {
    return !m_d->mouseMotionRequesters.isEmpty();
}

void Terminal::findFocusableItems(Item* root, QList<Item*>& list) {
    if (!root || !root->isEffectivelyVisible() || !root->isEffectivelyEnabled()) return;

    if (root->focusPolicy() & Qt::TabFocus) {
        list.append(root);
    }

    for (Item* child : root->childItems()) {
        findFocusableItems(child, list);
    }
}

void Terminal::setWidth(int) {
    qmlWarning(this) << "Terminal width is managed by the session; size an inner Item instead.";
}

void Terminal::setHeight(int) {
    qmlWarning(this) << "Terminal height is managed by mode; size an inner Item instead.";
}

Terminal::Mode Terminal::mode() const {
    return m_d->mode;
}

void Terminal::setMode(Mode mode) {
    if (m_d->mode == mode) return;
    m_d->mode = mode;

    if (!m_d->mouseReportingExplicit) {
        bool autoReporting = (m_d->mode == Mode::AltScreen);
        if (m_d->mouseReporting != autoReporting) {
            m_d->mouseReporting = autoReporting;
            emit mouseReportingChanged();
            if (m_d->engine) {
                m_d->engine->setMouseReporting(m_d->mouseReporting);
            }
        }
    }

    if (m_d->engine) {
        if (m_d->mode == Mode::AltScreen) {
            m_d->engine->setRenderer(new AltScreenRenderer());
        } else {
            m_d->engine->setRenderer(new InlineRenderer());
        }
    }

    invalidateSubtreeBounds();
    if (m_d->engine) m_d->engine->markFullDamage();
    emit modeChanged();
    emit heightChanged();
    markDirty();
}

Terminal::ClipboardMode Terminal::clipboardMode() const {
    return m_d->clipboardMode;
}

void Terminal::setClipboardMode(ClipboardMode mode) {
    if (m_d->clipboardMode == mode) return;
    m_d->clipboardMode = mode;
    if (mode == ClipboardMode::Disabled && m_d->engine) m_d->engine->cancelClipboardRead();
    emit clipboardModeChanged();
}

bool Terminal::mouseReporting() const {
    return m_d->mouseReporting;
}

void Terminal::setMouseReporting(bool enable) {
    m_d->mouseReportingExplicit = true;
    if (m_d->mouseReporting == enable) return;
    m_d->mouseReporting = enable;
    emit mouseReportingChanged();
    if (m_d->engine) {
        m_d->engine->setMouseReporting(enable);
    }
}

int Terminal::width() const {
    return terminalWidth();
}

int Terminal::implicitHeight() const {
    const int explicitImplicit = Item::implicitHeight();
    if (explicitImplicit > 0) return explicitImplicit;

    int maxBottom = 0;

    for (Item* child : childItems()) {
        if (!child || !child->visible()) continue;
        int childH = child->height() > 0 ? child->height() : child->implicitHeight();
        maxBottom = (std::max)(maxBottom, child->y() + childH);
    }
    return maxBottom;
}

int Terminal::height() const {
    if (m_d->mode == Mode::Inline)
        return std::clamp(implicitHeight(), 0, (std::max)(0, terminalHeight() - 1));
    return terminalHeight();
}

int Terminal::terminalWidth() const {
    return m_d->engine ? m_d->engine->terminalWidth() : 80;
}

int Terminal::terminalHeight() const {
    return m_d->engine ? m_d->engine->terminalHeight() : 24;
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
    if (m_d->engine) m_d->engine->clearHardwareCursorPosition();
    if (!visible()) return;

    // Nothing is behind the root, so a terminal-default background has nothing
    // to occlude: skipping the fill leaves the cells at their default and emits
    // fewer bytes for the same picture.
    if (m_d->background == 0 || Color::isTransparent(m_d->background)) return;
    const int left = std::max({0, absX, clipRect.x});
    const int right = std::min({buffer.width(), absX + width(), clipRect.x + clipRect.width});
    const int top = std::max({0, absY, clipRect.y});
    const int bottom = std::min({buffer.height(), absY + height(), clipRect.y + clipRect.height});
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            Cell& cell = buffer.cellAt(x, y);
            cell.bgColor = m_d->background;
            cell.setCharacter(" ");
            cell.displayWidth = 1;
            cell.continuation = false;
        }
    }
}

void Terminal::paintOverlay(TextGridBuffer& buffer, int absX, int absY,
                            const ClipRect& clipRect) const {
    Q_UNUSED(absX);
    Q_UNUSED(absY);
    for (Popup* popup : orderedPopups()) {
        if (popup && popup->visible()) {
            if (popup->modal() && popup->dimBackground()) {
                dimBuffer(buffer, clipRect);
            }

            QPoint absPos = popup->absolutePosition();
            // Pass the absolute terminal bounds as clipRect (clipRect is usually full screen)
            ItemPaintTraversal::paintSubtree(popup, buffer, absPos.x(), absPos.y(), clipRect);
        }
    }
}

} // namespace qmltui
