#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/FocusScope.h>
#include <QmlTui/Core/Items/AnchorLine.h>
#include <QmlTui/Core/Engine/Keys.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/Popup.h>
#include <QmlTui/Core/Engine/RenderPhase_p.h>
#include <QmlTui/Core/Engine/State.h>
#include <QmlTui/Core/Engine/Transition.h>
#include <QmlTui/Core/Engine/Animation.h>
#include <QmlTui/Core/Engine/StateController_p.h>
#include <QSet>
#include <QChildEvent>
#include <QPointer>
#include <QVarLengthArray>
#include <algorithm>

namespace qmltui {

Item::Item(QObject* parent) : QObject(parent), m_anchors(this) {
    if (parent) {
        if (Item* parentItem = qobject_cast<Item*>(parent)) {
            parentItem->registerChildItem(this);
        }
    }
}

Item::~Item() {
    // Anchors is a value member that is also parented to this QObject. Detach it
    // while Item's child-tracking containers are still alive; otherwise its
    // destructor sends a ChildRemoved event after those containers are gone.
    m_anchors.setParent(nullptr);

    Terminal* term = terminal();
    if (term && term->focusedItem() == this) {
        term->setFocusedItem(nullptr);
    }
}

AnchorLine* Item::left() const {
    if (!m_leftLine) m_leftLine = new AnchorLine(AnchorLine::Left, const_cast<Item*>(this));
    return m_leftLine;
}

AnchorLine* Item::right() const {
    if (!m_rightLine) m_rightLine = new AnchorLine(AnchorLine::Right, const_cast<Item*>(this));
    return m_rightLine;
}

AnchorLine* Item::top() const {
    if (!m_topLine) m_topLine = new AnchorLine(AnchorLine::Top, const_cast<Item*>(this));
    return m_topLine;
}

AnchorLine* Item::bottom() const {
    if (!m_bottomLine) m_bottomLine = new AnchorLine(AnchorLine::Bottom, const_cast<Item*>(this));
    return m_bottomLine;
}

AnchorLine* Item::horizontalCenter() const {
    if (!m_horizontalCenterLine)
        m_horizontalCenterLine =
            new AnchorLine(AnchorLine::HorizontalCenter, const_cast<Item*>(this));
    return m_horizontalCenterLine;
}

AnchorLine* Item::verticalCenter() const {
    if (!m_verticalCenterLine)
        m_verticalCenterLine = new AnchorLine(AnchorLine::VerticalCenter, const_cast<Item*>(this));
    return m_verticalCenterLine;
}

QQmlListProperty<QObject> Item::data() const {
    return QQmlListProperty<QObject>(const_cast<Item*>(this), nullptr, &Item::appendData,
                                     &Item::dataCount, &Item::dataAt, &Item::clearData);
}

void Item::appendData(QQmlListProperty<QObject>* list, QObject* obj) {
    Item* self = qobject_cast<Item*>(list->object);
    if (self && obj) {
        obj->setParent(self);
        self->m_dataChildren.append(obj);
        if (Item* item = qobject_cast<Item*>(obj)) {
            self->registerChildItem(item);
        }
    }
}

qsizetype Item::dataCount(QQmlListProperty<QObject>* list) {
    Item* self = qobject_cast<Item*>(list->object);
    return self ? self->m_dataChildren.size() : 0;
}

QObject* Item::dataAt(QQmlListProperty<QObject>* list, qsizetype index) {
    Item* self = qobject_cast<Item*>(list->object);
    return (self && index >= 0 && index < self->m_dataChildren.size())
               ? self->m_dataChildren.at(index).data()
               : nullptr;
}

void Item::clearData(QQmlListProperty<QObject>* list) {
    Item* self = qobject_cast<Item*>(list->object);
    if (self) {
        const QList<QPointer<QObject>> children = self->m_dataChildren;
        self->m_dataChildren.clear();
        for (const QPointer<QObject>& child : children)
            if (child && child->parent() == self) child->setParent(nullptr);
    }
}

QQmlListProperty<Item> Item::childrenProperty() const {
    return QQmlListProperty<Item>(
        const_cast<Item*>(this), nullptr,
        [](QQmlListProperty<Item>* list) -> qsizetype {
            Item* self = qobject_cast<Item*>(list->object);
            return self ? self->childItems().size() : 0;
        },
        [](QQmlListProperty<Item>* list, qsizetype index) -> Item* {
            Item* self = qobject_cast<Item*>(list->object);
            return (self && index >= 0 && index < self->childItems().size())
                       ? self->childItems().at(index)
                       : nullptr;
        });
}

void Item::setParentItem(Item* parentItem) {
    if (this->parentItem() == parentItem) {
        // An object QML created for an Item-typed property (Button.background,
        // contentItem) already has this parent, but the ChildAdded event ran
        // while it was still a bare QObject, so childEvent() could not see an
        // Item. Assigning the parent again is how such a child is adopted.
        if (parentItem && !parentItem->childItems().contains(this)) {
            parentItem->registerChildItem(this);
            markDirty();
        }
        return;
    }
    setParent(parentItem);
    emit parentChanged();
    // On attach registerChildItem already notified (via ChildAdded); this is
    // the path that reaches the subtree on a detach, where ChildRemoved fires
    // while the old parent is still in place and terminal() would resolve to
    // the tree the child just left.
    notifySubtreeAncestryChanged();
    markDirty();
}

void Item::setX(int x) {
    if (m_x == x) return;
    m_x = x;
    emit xChanged();
    markDirty();
}

void Item::setY(int y) {
    if (m_y == y) return;
    m_y = y;
    emit yChanged();
    markDirty();
}

void Item::setZ(qreal z) {
    if (qFuzzyCompare(m_z, z)) return;
    m_z = z;
    if (Item* p = parentItem()) p->invalidateStackingOrder();
    emit zChanged();
    markDirty();
}

void Item::invalidateStackingOrder() {
    m_stackingOrderDirty = true;
}

const QList<Item*>& Item::stackingChildren() const {
    if (!m_stackingOrderDirty) return m_stackingChildren;
    m_stackingChildren = m_visualChildren;
    std::stable_sort(
        m_stackingChildren.begin(), m_stackingChildren.end(),
        [](const Item* a, const Item* b) { return a && b ? a->z() < b->z() : a != nullptr; });
    m_stackingOrderDirty = false;
    return m_stackingChildren;
}

// A size change requires the item to lay out its children again, so geometry
// setters mark LayoutDirty as well as SelfDirty.
void Item::setWidth(int width) {
    if (m_width == width) return;
    m_width = width;
    emit widthChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void Item::setHeight(int height) {
    if (m_height == height) return;
    m_height = height;
    emit heightChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void Item::setImplicitWidth(int w) {
    if (m_implicitWidth == w) return;
    m_implicitWidth = w;
    m_reportedImplicitWidth = implicitWidth();
    emit implicitWidthChanged();
    if (m_width < 0) {
        emit widthChanged();
        markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    }
}

void Item::setImplicitHeight(int h) {
    if (m_implicitHeight == h) return;
    m_implicitHeight = h;
    m_reportedImplicitHeight = implicitHeight();
    emit implicitHeightChanged();
    if (m_height < 0) {
        emit heightChanged();
        markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    }
}

// Deliberately emits only the implicit-size signals, never widthChanged /
// heightChanged: QML "width: parent.width" bindings in children of an
// implicitly sized container would otherwise feed back into the container's
// implicit size and loop.
void Item::notifyImplicitSizeIfChanged() {
    const int width = implicitWidth();
    const int height = implicitHeight();
    if (width != m_reportedImplicitWidth) {
        m_reportedImplicitWidth = width;
        emit implicitWidthChanged();
    }
    if (height != m_reportedImplicitHeight) {
        m_reportedImplicitHeight = height;
        emit implicitHeightChanged();
    }
}

void Item::setVisible(bool visible) {
    if (m_visible == visible) return;
    m_visible = visible;
    emit visibleChanged();
    if (!visible) {
        Terminal* term = terminal();
        if (term) {
            for (Item* current = term->focusedItem(); current; current = current->parentItem()) {
                if (current == this) {
                    term->setFocusedItem(nullptr);
                    break;
                }
            }
        }
    }
    markDirty();
}

void Item::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    m_enabled = enabled;
    emit enabledChanged();
    if (!enabled) {
        Terminal* term = terminal();
        if (term) {
            for (Item* current = term->focusedItem(); current; current = current->parentItem()) {
                if (current == this) {
                    term->setFocusedItem(nullptr);
                    break;
                }
            }
        }
    }
    markDirty();
}

bool Item::isEffectivelyEnabled() const {
    for (const Item* current = this; current; current = current->parentItem()) {
        if (!current->enabled()) return false;
    }
    return true;
}

bool Item::isEffectivelyVisible() const {
    for (const Item* current = this; current; current = current->parentItem()) {
        if (!current->visible()) return false;
    }
    return true;
}

void Item::markDirty(DirtyFlags flags) {
    m_dirtyFlags |= flags;

    // Inside a frame the flags are enough: the layout pass is followed by the
    // paint pass, and paint-time invalidations must not schedule another
    // frame. Only idle invalidations notify the engine.
    const RenderPhase::Phase phase = RenderPhase::current();
    if (phase == RenderPhase::Paint) RenderPhase::recordDirtyDuringPaint(this);
    const bool notify = phase == RenderPhase::Idle;

    if (notify) emit dirty();
    // Bubble DescendantDirty (and DescendantLayoutDirty when LayoutDirty) up to ancestors
    const DirtyFlags propFlags =
        DirtyFlag::DescendantDirty |
        (flags.testFlag(DirtyFlag::LayoutDirty) ? DirtyFlag::DescendantLayoutDirty
                                                : DirtyFlag::None);
    Item* p = qobject_cast<Item*>(parent());
    while (p) {
        if ((p->m_dirtyFlags & propFlags) == propFlags && !notify) break;
        p->m_dirtyFlags |= propFlags;
        if (notify) emit p->dirty();
        p = qobject_cast<Item*>(p->parent());
    }
}

void Item::clearDirty(DirtyFlags flags) {
    m_dirtyFlags &= ~flags;
}

FocusScope* Item::enclosingFocusScope() const {
    for (Item* p = parentItem(); p; p = p->parentItem()) {
        if (p->isFocusScope()) {
            return static_cast<FocusScope*>(p);
        }
    }
    return nullptr;
}

void Item::setFocus(bool focus) {
    if (focus && (!isEffectivelyVisible() || !isEffectivelyEnabled())) return;

    FocusScope* scope = enclosingFocusScope();

    if (m_focus == focus) {
        if (focus) {
            if (scope) {
                scope->setSubFocusItem(this);
            }
            Terminal* term = terminal();
            if (term && term->focusedItem() != this && (!scope || scope->activeFocus())) {
                term->setFocusedItem(this);
            }
        }
        return;
    }
    m_focus = focus;
    emit focusChanged();

    if (scope) {
        if (m_focus) {
            scope->setSubFocusItem(this);
        } else if (scope->subFocusItem() == this) {
            scope->setSubFocusItem(nullptr);
        }
    }

    Terminal* term = terminal();
    if (term) {
        if (m_focus) {
            if (!scope || scope->activeFocus()) {
                term->setFocusedItem(this);
            }
        } else if (term->focusedItem() == this) {
            term->setFocusedItem(nullptr);
        }
    }
}

void Item::setActiveFocus(bool active) {
    if (m_activeFocus == active) return;
    m_activeFocus = active;
    emit activeFocusChanged();
    markDirty();
}

void Item::setFocusPolicy(Qt::FocusPolicy policy) {
    if (m_focusPolicy == policy) return;
    m_focusPolicy = policy;
    emit focusPolicyChanged();
    Terminal* term = terminal();
    if (m_focusPolicy == Qt::NoFocus && term && term->focusedItem() == this)
        term->setFocusedItem(nullptr);
}

void Item::forceActiveFocus() {
    setFocus(true);
    if (Terminal* term = terminal()) {
        term->setFocusedItem(this);
    }
}

// Depth-first: the first focusable descendant that asked for focus.
Item* Item::findFocusRequest() const {
    for (Item* child : m_visualChildren) {
        if (child->isFocusScope()) {
            auto* scope = static_cast<FocusScope*>(child);
            if (scope->focus()) {
                if (Item* sub = scope->subFocusItem()) {
                    if (Item* nested = sub->findFocusRequest()) return nested;
                    return sub;
                }
                if (Item* nested = scope->findFocusRequest()) return nested;
                return scope;
            }
            continue;
        }
        if (child->m_focus && child->m_focusPolicy != Qt::NoFocus) return child;
        if (Item* nested = child->findFocusRequest()) return nested;
    }
    return nullptr;
}

void Item::applyPendingFocus() {
    Terminal* term = isTerminalRoot() ? static_cast<Terminal*>(this) : terminal();
    if (!term || term->focusedItem()) return;
    if (FocusScope* scope =
            isFocusScope() ? static_cast<FocusScope*>(this) : enclosingFocusScope()) {
        if (!scope->focus() || (scope->enclosingFocusScope() && !scope->activeFocus())) return;
    }
    if (Item* request = findFocusRequest()) term->setFocusedItem(request);
}

Terminal* Item::terminal() const {
    QObject* p = parent();
    while (p) {
        Terminal* term = qobject_cast<Terminal*>(p);
        if (term) return term;
        p = p->parent();
    }
    return nullptr;
}

// Scrolling containers (ScrollView) override this; a plain Item never offsets
// its children. The previous dynamic-property lookup ran per item per frame.
QPoint Item::contentOffset() const {
    return QPoint();
}

QPoint Item::absolutePosition() const {
    int ax = m_x;
    int ay = m_y;

    Item* p = qobject_cast<Item*>(parent());
    while (p) {
        // If parent is a Terminal element, we do NOT add its x and y,
        // because Terminal is the screen root. Its x/y is 0 relative to screen.
        if (p->isTerminalRoot()) {
            break;
        }

        ax += p->x();
        ay += p->y();

        QPoint offset = p->contentOffset();
        ax -= offset.x();
        ay -= offset.y();

        p = qobject_cast<Item*>(p->parent());
    }
    return QPoint(ax, ay);
}

void Item::childEvent(QChildEvent* event) {
    QObject::childEvent(event);
    if (event->added()) {
        if (Item* childItem = qobject_cast<Item*>(event->child())) {
            registerChildItem(childItem);
            if (childItem->focus()) {
                childItem->setFocus(true);
            }
        }
    } else if (event->removed()) {
        QObject* childObject = event->child();
        m_dataChildren.removeIf([childObject](const QPointer<QObject>& entry) {
            return entry.isNull() || entry.data() == childObject;
        });
        if (m_stateController) {
            if (State* state = qobject_cast<State*>(childObject))
                m_stateController->stateRemoved(state);
            else if (Transition* transition = qobject_cast<Transition*>(childObject))
                m_stateController->transitionRemoved(transition);
        }
        if (Item* childItem = qobject_cast<Item*>(event->child())) {
            unregisterChildItem(childItem);
        } else {
            QObject* childObj = event->child();
            for (auto it = m_visualChildren.begin(); it != m_visualChildren.end(); ++it) {
                if (*it == childObj) {
                    Item* childItem = *it;
                    unregisterChildItem(childItem);
                    break;
                }
            }
        }
    }
}

void Item::registerChildItem(Item* childItem) {
    if (!childItem || m_visualChildren.contains(childItem)) return;
    m_visualChildren.append(childItem);
    invalidateStackingOrder();

    auto childGeometryChanged = [this]() { scheduleChildGeometryInvalidation(); };
    QList<QMetaObject::Connection> connections;
    connections.append(connect(childItem, &Item::widthChanged, this, childGeometryChanged));
    connections.append(connect(childItem, &Item::heightChanged, this, childGeometryChanged));
    connections.append(connect(childItem, &Item::implicitWidthChanged, this, childGeometryChanged));
    connections.append(
        connect(childItem, &Item::implicitHeightChanged, this, childGeometryChanged));
    connections.append(connect(childItem, &Item::visibleChanged, this, childGeometryChanged));
    if (isTerminalRoot())
        connections.append(connect(childItem, &Item::yChanged, this, childGeometryChanged));
    m_childGeometryConnections.insert(childItem, connections);

    scheduleChildGeometryInvalidation();
    applyPendingFocus();
    childItem->notifySubtreeAncestryChanged();
    emit childrenChanged();
}

void Item::notifySubtreeAncestryChanged() {
    emit ancestryChanged();
    const QList<Item*> children = m_visualChildren;
    for (Item* child : children)
        if (child) child->notifySubtreeAncestryChanged();
}

void Item::reorderChildItemsAfter(Item* sibling, const QList<Item*>& items) {
    if (!m_visualChildren.contains(sibling) || items.isEmpty()) return;
    const QSet<Item*> generated(items.cbegin(), items.cend());
    QList<Item*> ordered;
    ordered.reserve(m_visualChildren.size());
    for (Item* child : std::as_const(m_visualChildren)) {
        if (!generated.contains(child)) ordered.append(child);
        if (child == sibling) {
            for (Item* item : items)
                if (item && item->parentItem() == this) ordered.append(item);
        }
    }
    if (ordered == m_visualChildren) return;
    m_visualChildren = std::move(ordered);
    invalidateStackingOrder();
    scheduleChildGeometryInvalidation();
}

void Item::unregisterChildItem(Item* childItem) {
    if (!childItem) return;
    if (m_visualChildren.removeOne(childItem)) {
        const auto connections = m_childGeometryConnections.take(childItem);
        for (const QMetaObject::Connection& connection : connections)
            disconnect(connection);
        invalidateStackingOrder();
        emit childrenChanged();
        // The child may be mid-destruction or mid-reparent and subclasses'
        // implicitWidth() overrides may still list it, so do not evaluate the
        // implicit size here: report unconditionally and make the next
        // notifyImplicitSizeIfChanged() emit regardless.
        markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
        m_reportedImplicitWidth = -1;
        m_reportedImplicitHeight = -1;
        emit implicitWidthChanged();
        emit implicitHeightChanged();
    }
}

// A child's geometry changed: relayout, and tell dependants only if this
// item's implicit size actually moved. Unconditional emission here used to
// cascade through every ancestor and every anchored sibling per setter call.
void Item::scheduleChildGeometryInvalidation() {
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}
// Top-down layout pass. Runs updateLayout() wherever LayoutDirty is set, then
// recurses into children, and re-runs this item's layout once if a child's
// layout changed this item's implicit size. Iterations are bounded so an
// updateLayout() that keeps invalidating itself cannot hang the frame.
void Item::layoutSubtree() {
    if (!m_visible) return;
    if (!(m_dirtyFlags & (DirtyFlag::LayoutDirty | DirtyFlag::DescendantLayoutDirty))) return;
    m_dirtyFlags &= ~DirtyFlags(DirtyFlag::DescendantLayoutDirty);

    QPointer<Item> selfGuard(this);
    constexpr int MaxLayoutPasses = 3;

    for (int pass = 0; pass < MaxLayoutPasses && m_dirtyFlags.testFlag(DirtyFlag::LayoutDirty);
         ++pass) {
        m_dirtyFlags &= ~DirtyFlags(DirtyFlag::LayoutDirty);
        updateLayout();
        if (!selfGuard) return;
    }

    const QList<Item*>& orderedChildren = stackingChildren();
    QVarLengthArray<QPointer<Item>, 64> children;
    children.reserve(orderedChildren.size());
    for (Item* child : orderedChildren)
        children.append(child);
    for (const QPointer<Item>& childGuard : children) {
        if (Item* childItem = childGuard.data()) childItem->layoutSubtree();
        if (!selfGuard) return;
    }

    if (m_dirtyFlags.testFlag(DirtyFlag::LayoutDirty)) {
        m_dirtyFlags &= ~DirtyFlags(DirtyFlag::LayoutDirty);
        updateLayout();
    }
    m_dirtyFlags &= ~DirtyFlags(DirtyFlag::DescendantLayoutDirty);
}

void Item::paintSubtree(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;

    QPointer<Item> selfGuard(this);

    // Layout normally ran in layoutSubtree(); this handles items painted
    // without a preceding layout pass and layouts invalidated by a sibling's
    // paint.
    const DirtyFlags consumedFlags = m_dirtyFlags;
    m_dirtyFlags &= ~consumedFlags;
    if (consumedFlags.testFlag(DirtyFlag::LayoutDirty)) {
        updateLayout();
        if (!selfGuard) return;
    }

    paint(buffer, absX, absY, clipRect);
    if (!selfGuard) return;

    const ClipRect childrenClip = clipRectForChildren(absX, absY, clipRect);
    if (!m_clip || !childrenClip.isEmpty()) {
        const QPoint offset = contentOffset();

        const QList<Item*>& orderedChildren = stackingChildren();
        QVarLengthArray<QPointer<Item>, 64> children;
        children.reserve(orderedChildren.size());
        for (Item* child : orderedChildren)
            children.append(child);
        for (const QPointer<Item>& childGuard : children) {
            Item* childItem = childGuard.data();
            if (!childItem) continue;
            if (childItem->isPopup() && childItem->visible())
                continue; // painted by Terminal::paintOverlay
            const int childAbsX = absX + childItem->x() - offset.x();
            const int childAbsY = absY + childItem->y() - offset.y();
            const int childW = childItem->width();
            const int childH = childItem->height();
            if ((childItem->clip() || childItem->childItems().isEmpty()) &&
                !childItem->m_dirtyFlags.testFlag(DirtyFlag::LayoutDirty) && childW > 0 &&
                childH > 0) {
                if (childrenClip.intersected(ClipRect{childAbsX, childAbsY, childW, childH})
                        .isEmpty()) {
                    childItem->m_dirtyFlags &=
                        ~(DirtyFlags(DirtyFlag::SelfDirty) | DirtyFlag::DescendantDirty);
                    continue;
                }
            }
            childItem->paintSubtree(buffer, childAbsX, childAbsY, childrenClip);
            if (!selfGuard) return;
        }
    }

    paintOverlay(buffer, absX, absY, clipRect);
}

void Item::updateLayout() {}

void Item::paint(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) const {
    Q_UNUSED(buffer);
    Q_UNUSED(absX);
    Q_UNUSED(absY);
    Q_UNUSED(clipRect);
}

void Item::paintOverlay(TextGridBuffer& buffer, int absX, int absY,
                        const ClipRect& clipRect) const {
    Q_UNUSED(buffer);
    Q_UNUSED(absX);
    Q_UNUSED(absY);
    Q_UNUSED(clipRect);
}

ClipRect Item::clipRectForChildren(int absX, int absY, const ClipRect& clipRect) const {
    if (!m_clip) return clipRect;
    return clipRect.intersected(ClipRect{absX, absY, width(), height()});
}

int Item::width() const {
    return m_width >= 0 ? m_width : implicitWidth();
}

int Item::height() const {
    return m_height >= 0 ? m_height : implicitHeight();
}

void Item::setPadding(int p) {
    if (m_padding == p) return;
    m_padding = p;
    emit paddingChanged();
    if (m_leftPadding < 0) emit leftPaddingChanged();
    if (m_rightPadding < 0) emit rightPaddingChanged();
    if (m_topPadding < 0) emit topPaddingChanged();
    if (m_bottomPadding < 0) emit bottomPaddingChanged();
    markDirty();
}

void Item::setClip(bool c) {
    if (m_clip == c) return;
    m_clip = c;
    emit clipChanged();
    markDirty();
}

int Item::leftPadding() const {
    return m_leftPadding >= 0 ? m_leftPadding : m_padding;
}

void Item::setLeftPadding(int p) {
    if (m_leftPadding == p) return;
    m_leftPadding = p;
    emit leftPaddingChanged();
    markDirty();
}

int Item::rightPadding() const {
    return m_rightPadding >= 0 ? m_rightPadding : m_padding;
}

void Item::setRightPadding(int p) {
    if (m_rightPadding == p) return;
    m_rightPadding = p;
    emit rightPaddingChanged();
    markDirty();
}

int Item::topPadding() const {
    return m_topPadding >= 0 ? m_topPadding : m_padding;
}

void Item::setTopPadding(int p) {
    if (m_topPadding == p) return;
    m_topPadding = p;
    emit topPaddingChanged();
    markDirty();
}

int Item::bottomPadding() const {
    return m_bottomPadding >= 0 ? m_bottomPadding : m_padding;
}

void Item::setBottomPadding(int p) {
    if (m_bottomPadding == p) return;
    m_bottomPadding = p;
    emit bottomPaddingChanged();
    markDirty();
}

void Item::keyPressEvent(class KeyEvent* event) {
    Q_UNUSED(event);
}

void Item::mousePressEvent(class MouseEvent* event) {
    Q_UNUSED(event);
}

void Item::mouseReleaseEvent(class MouseEvent* event) {
    Q_UNUSED(event);
}

void Item::mouseMoveEvent(class MouseEvent* event) {
    Q_UNUSED(event);
}

void Item::wheelEvent(class MouseEvent* event) {
    Q_UNUSED(event);
}

bool Item::childMouseEventFilter(Item* item, class MouseEvent* event) {
    Q_UNUSED(item);
    Q_UNUSED(event);
    return false;
}

StateController* Item::ensureStateController() const {
    if (!m_stateController) {
        auto* self = const_cast<Item*>(this);
        m_stateController = std::make_unique<StateController>(self);
        m_stateController->setDefaultTarget(self);
        connect(m_stateController.get(), &StateController::stateChanged, self, &Item::stateChanged);
    }
    return m_stateController.get();
}

QQmlListProperty<qmltui::State> Item::states() const {
    return ensureStateController()->statesProperty();
}

QQmlListProperty<qmltui::Transition> Item::transitions() const {
    return ensureStateController()->transitionsProperty();
}

QString Item::state() const {
    return m_stateController ? m_stateController->state() : QString();
}

void Item::setState(const QString& newState) {
    if (!m_stateController && newState.isEmpty()) return;
    ensureStateController()->setState(newState);
}

void Item::evaluateStateConditions() {
    if (m_stateController) m_stateController->evaluateStateConditions();
}

} // namespace qmltui
