#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/Item_p.h>
#include <QmlTui/Core/Items/FocusScope.h>
#include <QmlTui/Core/Items/Column.h>
#include <QmlTui/Core/Items/GridLayout.h>
#include <QmlTui/Core/Items/Loader.h>
#include <QmlTui/Core/Items/MouseArea.h>
#include <QmlTui/Core/Items/Repeater.h>
#include <QmlTui/Core/Items/Row.h>
#include <QmlTui/Core/Items/Scrollback.h>
#include <QmlTui/Core/Items/StackLayout.h>
#include <QmlTui/Core/Items/AnchorLine.h>
#include <QmlTui/Core/Engine/KeyEvent.h>
#include <QmlTui/Core/Engine/MouseEvent.h>
#include <QmlTui/Core/Engine/TerminalEngine.h>
#include <QmlTui/Core/Items/Terminal.h>
#include <QmlTui/Core/Items/Popup.h>
#include <QmlTui/Core/Engine/RenderPhase_p.h>
#include <QmlTui/Core/Engine/State.h>
#include <QmlTui/Core/Engine/Transition.h>
#include <qqmlprivate.h>
#include <QSet>
#include <QChildEvent>
#include <QPointer>
#include <QSignalBlocker>
#include <QVarLengthArray>
#include <algorithm>
#include <typeinfo>

namespace qmltui {

Item::Item(QObject* parent) : QObject(parent), m_d(std::make_unique<ItemPrivate>()) {
    if (parent) {
        if (Item* parentItem = qobject_cast<Item*>(parent)) {
            parentItem->registerChildItem(this);
        }
    }
}

Item::~Item() {
    setItemFlag(ItemFlag::InDestructor, true);
    // Anchors and StateController are owned members that are also parented to
    // this QObject. Detach and destroy them while Item is still alive.
    if (std::unique_ptr<Anchors> anchors = std::move(m_d->anchors)) {
        anchors->setParent(nullptr);
    }
    if (std::unique_ptr<StateController> stateController = std::move(m_d->stateController)) {
        stateController->setParent(nullptr);
    }
    for (Item* child : std::as_const(m_d->visualChildren)) {
        if (child && child->m_d->parentItem == this) child->m_d->parentItem = nullptr;
    }

    if (!isTerminalRoot()) {
        if (Terminal* term = terminal()) {
            if (term->focusedItem() == this) {
                QSignalBlocker blocker(this);
                term->setFocusedItem(nullptr);
            }
            damageLastPaintedSubtree();
        }
    }
    m_d->parentItem = nullptr;
}

const QList<Item*>& Item::childItems() const {
    return m_d->visualChildren;
}

int Item::x() const {
    return m_d->x;
}

int Item::y() const {
    return m_d->y;
}

qreal Item::z() const {
    return m_d->z;
}

int Item::implicitWidth() const {
    return m_d->implicitWidth;
}

int Item::implicitHeight() const {
    return m_d->implicitHeight;
}

bool Item::visible() const {
    return m_d->visible;
}

bool Item::enabled() const {
    return m_d->enabled;
}

bool Item::focus() const {
    return m_d->focus;
}

bool Item::activeFocus() const {
    return m_d->activeFocus;
}

Qt::FocusPolicy Item::focusPolicy() const {
    return m_d->focusPolicy;
}

Item* Item::parentItem() const {
    if (m_d->parentItem) return m_d->parentItem;
    QObject* objParent = QObject::parent();
    return objParent ? qobject_cast<Item*>(objParent) : nullptr;
}

Anchors* Item::anchors() const {
    if (!m_d->anchors) {
        m_d->anchors = std::make_unique<Anchors>(const_cast<Item*>(this));
    }
    return m_d->anchors.get();
}

const Anchors* Item::anchorsIfAny() const noexcept {
    return m_d->anchors.get();
}

AnchorLine* Item::left() const {
    if (!m_d->leftLine) m_d->leftLine = new AnchorLine(AnchorLine::Left, const_cast<Item*>(this));
    return m_d->leftLine;
}

AnchorLine* Item::right() const {
    if (!m_d->rightLine)
        m_d->rightLine = new AnchorLine(AnchorLine::Right, const_cast<Item*>(this));
    return m_d->rightLine;
}

AnchorLine* Item::top() const {
    if (!m_d->topLine) m_d->topLine = new AnchorLine(AnchorLine::Top, const_cast<Item*>(this));
    return m_d->topLine;
}

AnchorLine* Item::bottom() const {
    if (!m_d->bottomLine)
        m_d->bottomLine = new AnchorLine(AnchorLine::Bottom, const_cast<Item*>(this));
    return m_d->bottomLine;
}

AnchorLine* Item::horizontalCenter() const {
    if (!m_d->horizontalCenterLine)
        m_d->horizontalCenterLine =
            new AnchorLine(AnchorLine::HorizontalCenter, const_cast<Item*>(this));
    return m_d->horizontalCenterLine;
}

AnchorLine* Item::verticalCenter() const {
    if (!m_d->verticalCenterLine)
        m_d->verticalCenterLine =
            new AnchorLine(AnchorLine::VerticalCenter, const_cast<Item*>(this));
    return m_d->verticalCenterLine;
}

bool Item::clip() const {
    return m_d->clip;
}

int Item::padding() const {
    return m_d->padding;
}

bool Item::filtersChildMouseEvents() const {
    return m_d->filtersChildMouseEvents;
}

void Item::setFiltersChildMouseEvents(bool filter) {
    m_d->filtersChildMouseEvents = filter;
}

bool Item::isPopup() const {
    return m_d->itemFlags & static_cast<quint8>(ItemFlag::IsPopup);
}

bool Item::isTerminalRoot() const {
    return m_d->itemFlags & static_cast<quint8>(ItemFlag::IsTerminalRoot);
}

bool Item::hasExplicitX() const {
    return m_d->itemFlags & static_cast<quint8>(ItemFlag::ExplicitX);
}

bool Item::hasExplicitY() const {
    return m_d->itemFlags & static_cast<quint8>(ItemFlag::ExplicitY);
}

bool Item::hasExplicitWidth() const {
    return m_d->width >= 0;
}

bool Item::hasExplicitHeight() const {
    return m_d->height >= 0;
}

bool Item::ignoresParentPadding() const {
    return m_d->itemFlags & static_cast<quint8>(ItemFlag::IgnoresParentPadding);
}

Item::DirtyFlags Item::dirtyFlags() const {
    return m_d->dirtyFlags;
}

void Item::setItemFlag(ItemFlag flag, bool on) {
    if (on)
        m_d->itemFlags |= static_cast<quint8>(flag);
    else
        m_d->itemFlags &= static_cast<quint8>(~static_cast<quint8>(flag));
}

void Item::invalidateSubtreeBounds() {
    for (Item* cur = this; cur; cur = cur->parentItem()) {
        if (!cur->m_d->subtreeBoundsValid) break;
        cur->m_d->subtreeBoundsValid = false;
    }
}

void Item::clearFocusFlagAndNotify() {
    if (!m_d->focus) return;
    m_d->focus = false;
    emit focusChanged();
}

QQmlListProperty<QObject> Item::data() const {
    return QQmlListProperty<QObject>(const_cast<Item*>(this), nullptr, &Item::appendData,
                                     &Item::dataCount, &Item::dataAt, &Item::clearData);
}

void Item::appendData(QQmlListProperty<QObject>* list, QObject* obj) {
    Item* self = qobject_cast<Item*>(list->object);
    if (self && obj) {
        obj->setParent(self);
        self->m_d->dataChildren.append(obj);
        if (Item* item = qobject_cast<Item*>(obj)) {
            self->registerChildItem(item);
        }
    }
}

qsizetype Item::dataCount(QQmlListProperty<QObject>* list) {
    Item* self = qobject_cast<Item*>(list->object);
    return self ? self->m_d->dataChildren.size() : 0;
}

QObject* Item::dataAt(QQmlListProperty<QObject>* list, qsizetype index) {
    Item* self = qobject_cast<Item*>(list->object);
    return (self && index >= 0 && index < self->m_d->dataChildren.size())
               ? self->m_d->dataChildren.at(index).data()
               : nullptr;
}

void Item::clearData(QQmlListProperty<QObject>* list) {
    Item* self = qobject_cast<Item*>(list->object);
    if (self) {
        const QList<QPointer<QObject>> children = self->m_d->dataChildren;
        self->m_d->dataChildren.clear();
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
    damageLastPaintedSubtree();
    invalidateSubtreeBounds();
    setParent(parentItem);
    invalidateSubtreeBounds();
    emit parentChanged();
    // On attach registerChildItem already notified (via ChildAdded); this is
    // the path that reaches the subtree on a detach, where ChildRemoved fires
    // while the old parent is still in place and terminal() would resolve to
    // the tree the child just left.
    notifySubtreeAncestryChanged();
    markDirty();
}

bool Item::isDescendantOf(const Item* ancestor) const noexcept {
    if (!ancestor) return false;
    for (const QObject* p = this; p; p = p->parent()) {
        if (p == ancestor) return true;
    }
    return false;
}

void Item::setX(int x) {
    setItemFlag(ItemFlag::ExplicitX, true);
    setDefaultX(x);
}

void Item::setDefaultX(int x) {
    if (m_d->x == x) return;
    m_d->x = x;
    invalidateSubtreeBounds();
    emit xChanged();
    markDirty();
}

void Item::setY(int y) {
    setItemFlag(ItemFlag::ExplicitY, true);
    setDefaultY(y);
}

void Item::setDefaultY(int y) {
    if (m_d->y == y) return;
    m_d->y = y;
    invalidateSubtreeBounds();
    emit yChanged();
    if (Item* p = parentItem(); p && p->isTerminalRoot()) p->scheduleChildGeometryInvalidation();
    markDirty();
}

void Item::setZ(qreal z) {
    if (qFuzzyCompare(m_d->z, z)) return;
    m_d->z = z;
    if (Item* p = parentItem()) p->invalidateStackingOrder();
    emit zChanged();
    markDirty();
}

void Item::invalidateStackingOrder() {
    m_d->stackingOrderDirty = true;
}

const QList<Item*>& Item::stackingChildren() const {
    if (!m_d->stackingOrderDirty) return m_d->stackingChildren;
    m_d->stackingChildren = m_d->visualChildren;
    std::stable_sort(
        m_d->stackingChildren.begin(), m_d->stackingChildren.end(),
        [](const Item* a, const Item* b) { return a && b ? a->z() < b->z() : a != nullptr; });
    m_d->stackingOrderDirty = false;
    return m_d->stackingChildren;
}

// A size change requires the item to lay out its children again, so geometry
// setters mark LayoutDirty as well as SelfDirty.
void Item::setWidth(int width) {
    if (m_d->width == width) return;
    m_d->width = width;
    invalidateSubtreeBounds();
    emit widthChanged();
    if (Item* p = parentItem()) p->scheduleChildGeometryInvalidation();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void Item::setHeight(int height) {
    if (m_d->height == height) return;
    m_d->height = height;
    invalidateSubtreeBounds();
    emit heightChanged();
    if (Item* p = parentItem()) p->scheduleChildGeometryInvalidation();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
}

void Item::setImplicitWidth(int w) {
    if (m_d->implicitWidth == w) return;
    m_d->implicitWidth = w;
    m_d->reportedImplicitWidth = implicitWidth();
    emit implicitWidthChanged();
    if (m_d->width < 0) {
        invalidateSubtreeBounds();
        emit widthChanged();
        markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    }
    if (Item* p = parentItem()) p->scheduleChildGeometryInvalidation();
}

void Item::setImplicitHeight(int h) {
    if (m_d->implicitHeight == h) return;
    m_d->implicitHeight = h;
    m_d->reportedImplicitHeight = implicitHeight();
    emit implicitHeightChanged();
    if (m_d->height < 0) {
        invalidateSubtreeBounds();
        emit heightChanged();
        markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    }
    if (Item* p = parentItem()) p->scheduleChildGeometryInvalidation();
}

// Deliberately emits only the implicit-size signals, never widthChanged /
// heightChanged: QML "width: parent.width" bindings in children of an
// implicitly sized container would otherwise feed back into the container's
// implicit size and loop.
void Item::notifyImplicitSizeIfChanged() {
    const int width = implicitWidth();
    const int height = implicitHeight();
    bool changed = false;
    if (width != m_d->reportedImplicitWidth) {
        m_d->reportedImplicitWidth = width;
        emit implicitWidthChanged();
        changed = true;
    }
    if (height != m_d->reportedImplicitHeight) {
        m_d->reportedImplicitHeight = height;
        emit implicitHeightChanged();
        changed = true;
    }
    if (changed) {
        invalidateSubtreeBounds();
        if (Item* p = parentItem()) p->scheduleChildGeometryInvalidation();
    }
}

void Item::setVisible(bool visible) {
    if (m_d->visible == visible) return;
    if (!visible) {
        damageLastPaintedSubtree();
    }
    m_d->visible = visible;
    invalidateSubtreeBounds();
    emit visibleChanged();
    if (Item* p = parentItem()) p->scheduleChildGeometryInvalidation();
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
    if (m_d->enabled == enabled) return;
    m_d->enabled = enabled;
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
    m_d->dirtyFlags |= flags;
    if (flags & (DirtyFlag::LayoutDirty | DirtyFlag::DescendantLayoutDirty)) {
        invalidateSubtreeBounds();
    }

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
        ((flags & (DirtyFlag::LayoutDirty | DirtyFlag::DescendantLayoutDirty))
             ? DirtyFlag::DescendantLayoutDirty
             : DirtyFlag::None);
    Item* p = parentItem();
    while (p) {
        if ((p->m_d->dirtyFlags & propFlags) == propFlags && !notify) break;
        p->m_d->dirtyFlags |= propFlags;
        if (notify) emit p->dirty();
        p = p->parentItem();
    }
}

void Item::clearDirty(DirtyFlags flags) {
    m_d->dirtyFlags &= ~flags;
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

    if (m_d->focus == focus) {
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
    m_d->focus = focus;
    emit focusChanged();

    if (scope) {
        if (m_d->focus) {
            scope->setSubFocusItem(this);
        } else if (scope->subFocusItem() == this) {
            scope->setSubFocusItem(nullptr);
        }
    }

    Terminal* term = terminal();
    if (term) {
        if (m_d->focus) {
            if (!scope || scope->activeFocus()) {
                term->setFocusedItem(this);
            }
        } else if (term->focusedItem() == this) {
            term->setFocusedItem(nullptr);
        }
    }
}

void Item::setActiveFocus(bool active) {
    if (m_d->activeFocus == active) return;
    m_d->activeFocus = active;
    emit activeFocusChanged();
    markDirty();
}

void Item::setFocusPolicy(Qt::FocusPolicy policy) {
    if (m_d->focusPolicy == policy) return;
    m_d->focusPolicy = policy;
    emit focusPolicyChanged();
    Terminal* term = terminal();
    if (m_d->focusPolicy == Qt::NoFocus && term && term->focusedItem() == this)
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
    for (Item* child : m_d->visualChildren) {
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
        if (child->m_d->focus && child->m_d->focusPolicy != Qt::NoFocus) return child;
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
    const Item* cur = this;
    while (Item* p = cur->parentItem()) {
        if (p->isTerminalRoot()) return static_cast<Terminal*>(p);
        cur = p;
    }
    for (QObject* p = cur->parent(); p; p = p->parent()) {
        if (Terminal* term = qobject_cast<Terminal*>(p)) return term;
    }
    return nullptr;
}

// Scrolling containers (ScrollView) override this; a plain Item never offsets
// its children. The previous dynamic-property lookup ran per item per frame.
QPoint Item::contentOffset() const {
    return QPoint();
}

QPoint Item::absolutePosition() const {
    int ax = m_d->x;
    int ay = m_d->y;

    Item* p = parentItem();
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

        p = p->parentItem();
    }
    return QPoint(ax, ay);
}

void Item::childEvent(QChildEvent* event) {
    QObject::childEvent(event);
    if (m_d->itemFlags & static_cast<quint8>(ItemFlag::InDestructor)) return;
    if (event->added()) {
        if (Item* childItem = qobject_cast<Item*>(event->child())) {
            registerChildItem(childItem);
            if (childItem->focus()) {
                childItem->setFocus(true);
            }
        }
    } else if (event->removed()) {
        QObject* childObject = event->child();
        if (childObject == m_d->anchors.get()) {
            (void)m_d->anchors.release();
            return;
        }
        if (childObject == m_d->stateController.get()) {
            (void)m_d->stateController.release();
            return;
        }
        m_d->dataChildren.removeIf([childObject](const QPointer<QObject>& entry) {
            return entry.isNull() || entry.data() == childObject;
        });
        if (m_d->stateController) {
            if (State* state = qobject_cast<State*>(childObject))
                m_d->stateController->stateRemoved(state);
            else if (Transition* transition = qobject_cast<Transition*>(childObject))
                m_d->stateController->transitionRemoved(transition);
        }
        if (Item* childItem = qobject_cast<Item*>(childObject)) {
            childItem->damageLastPaintedSubtree();
            if (childItem->m_d->parentItem == this) childItem->m_d->parentItem = nullptr;
        }
        unregisterChildItem(childObject);
    }
}

void Item::registerChildItem(Item* childItem) {
    if (!childItem) return;
    Q_ASSERT(static_cast<const void*>(childItem) ==
             static_cast<const void*>(static_cast<const QObject*>(childItem)));
    childItem->m_d->parentItem = this;
    if (m_d->visualChildren.contains(childItem)) return;
    m_d->visualChildren.append(childItem);
    invalidateStackingOrder();
    invalidateSubtreeBounds();

    scheduleChildGeometryInvalidation();
    if (childItem->m_d->dirtyFlags & (DirtyFlag::LayoutDirty | DirtyFlag::DescendantLayoutDirty)) {
        markDirty(DirtyFlag::DescendantLayoutDirty);
    }
    applyPendingFocus();
    childItem->notifySubtreeAncestryChanged();
    emit childrenChanged();
}

void Item::notifySubtreeAncestryChanged() {
    emit ancestryChanged();
    const QList<Item*> children = m_d->visualChildren;
    for (Item* child : children)
        if (child) child->notifySubtreeAncestryChanged();
}

void Item::reorderChildItemsAfter(Item* sibling, const QList<Item*>& items) {
    if (!m_d->visualChildren.contains(sibling) || items.isEmpty()) return;
    const QSet<Item*> generated(items.cbegin(), items.cend());
    QList<Item*> ordered;
    ordered.reserve(m_d->visualChildren.size());
    for (Item* child : std::as_const(m_d->visualChildren)) {
        if (!generated.contains(child)) ordered.append(child);
        if (child == sibling) {
            for (Item* item : items)
                if (item && item->parentItem() == this) ordered.append(item);
        }
    }
    if (ordered == m_d->visualChildren) return;
    m_d->visualChildren = std::move(ordered);
    invalidateStackingOrder();
    scheduleChildGeometryInvalidation();
}

void Item::unregisterChildItem(QObject* childObject) {
    if (!childObject) return;
    bool removed = false;
    for (auto it = m_d->visualChildren.begin(); it != m_d->visualChildren.end(); ++it) {
        if (static_cast<const void*>(*it) == static_cast<const void*>(childObject)) {
            m_d->visualChildren.erase(it);
            removed = true;
            break;
        }
    }
    if (removed) {
        invalidateStackingOrder();
        invalidateSubtreeBounds();
        emit childrenChanged();
        // The child may be mid-destruction or mid-reparent and subclasses'
        // implicitWidth() overrides may still list it, so do not evaluate the
        // implicit size here: report unconditionally and make the next
        // notifyImplicitSizeIfChanged() emit regardless.
        markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
        m_d->reportedImplicitWidth = -1;
        m_d->reportedImplicitHeight = -1;
        emit implicitWidthChanged();
        emit implicitHeightChanged();
        if (Item* p = parentItem()) p->scheduleChildGeometryInvalidation();
    }
}

// A child's geometry changed: relayout, and tell dependants only if this
// item's implicit size actually moved. Unconditional emission here used to
// cascade through every ancestor and every anchored sibling per setter call.
void Item::scheduleChildGeometryInvalidation() {
    invalidateSubtreeBounds();
    markDirty(DirtyFlag::LayoutDirty | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

static bool isExactNonPaintingContainer(const Item* item) {
#if defined(__GXX_RTTI) || defined(_CPPRTTI)
    const std::type_info& ti = typeid(*item);
    return ti == typeid(Item) || ti == typeid(QQmlPrivate::QQmlElement<Item>) ||
           ti == typeid(FocusScope) || ti == typeid(QQmlPrivate::QQmlElement<FocusScope>) ||
           ti == typeid(Column) || ti == typeid(QQmlPrivate::QQmlElement<Column>) ||
           ti == typeid(Row) || ti == typeid(QQmlPrivate::QQmlElement<Row>) ||
           ti == typeid(GridLayout) || ti == typeid(QQmlPrivate::QQmlElement<GridLayout>) ||
           ti == typeid(StackLayout) || ti == typeid(QQmlPrivate::QQmlElement<StackLayout>) ||
           ti == typeid(Loader) || ti == typeid(QQmlPrivate::QQmlElement<Loader>) ||
           ti == typeid(Repeater) || ti == typeid(QQmlPrivate::QQmlElement<Repeater>) ||
           ti == typeid(MouseArea) || ti == typeid(QQmlPrivate::QQmlElement<MouseArea>) ||
           ti == typeid(Scrollback) || ti == typeid(QQmlPrivate::QQmlElement<Scrollback>);
#else
    Q_UNUSED(item);
    return false;
#endif
}

QRect Item::localSelfBounds() const {
    if (isExactNonPaintingContainer(this)) return QRect();
    const int w = width();
    const int h = height();
    if (m_d->itemFlags & static_cast<quint8>(ItemFlag::BoundedSelfPaint)) {
        if (w <= 0 || h <= 0) return QRect();
        const int extraRight =
            ((m_d->itemFlags & static_cast<quint8>(ItemFlag::ScrollbarOverhang)) &&
             rightPadding() == 0)
                ? 1
                : 0;
        return QRect(0, 0, w + extraRight, h);
    }
    const int effectiveW = w > 0 ? w : 16384;
    const int effectiveH = h > 0 ? h : 16384;
    return QRect(0, 0, effectiveW, effectiveH);
}

const QRect& Item::ensureSubtreeBounds() const {
    if (m_d->subtreeBoundsValid) return m_d->subtreeBounds;
    if (!m_d->visible) {
        m_d->subtreeBounds = QRect();
        m_d->subtreeBoundsValid = true;
        return m_d->subtreeBounds;
    }
    QRect bounds = localSelfBounds();
    if (!m_d->visualChildren.isEmpty()) {
        QRect childrenUnion;
        const QPoint offset = contentOffset();
        for (Item* child : m_d->visualChildren) {
            if (!child || !child->visible() || child->isPopup()) continue;
            const QRect& childBounds = child->ensureSubtreeBounds();
            if (!childBounds.isEmpty()) {
                childrenUnion |=
                    childBounds.translated(child->x() - offset.x(), child->y() - offset.y());
            }
        }
        if (!childrenUnion.isEmpty()) {
            if (m_d->clip && !isTerminalRoot()) {
                const ClipRect c =
                    clipRectForChildren(0, 0, ClipRect{-99999, -99999, 199998, 199998});
                childrenUnion &= QRect(c.x, c.y, c.width, c.height);
            }
            if (!childrenUnion.isEmpty()) bounds |= childrenUnion;
        }
    }
    m_d->subtreeBounds = bounds;
    m_d->subtreeBoundsValid = true;
    return m_d->subtreeBounds;
}

void Item::clearPaintDirtySubtree() {
    if (!(m_d->dirtyFlags & (DirtyFlag::SelfDirty | DirtyFlag::DescendantDirty))) return;
    m_d->dirtyFlags &= ~(DirtyFlags(DirtyFlag::SelfDirty) | DirtyFlag::DescendantDirty);
    for (Item* child : m_d->visualChildren) {
        if (child) child->clearPaintDirtySubtree();
    }
}

void Item::damageLastPaintedSubtree() {
    QRect painted;
    auto collect = [&](auto& self, Item* cur) -> void {
        if (!cur) return;
        if (!cur->m_d->lastPaintedSubtreeRect.isEmpty()) {
            painted |= cur->m_d->lastPaintedSubtreeRect;
            cur->m_d->lastPaintedSubtreeRect = QRect();
        }
        for (Item* child : cur->m_d->visualChildren) {
            self(self, child);
        }
    };
    collect(collect, this);
    if (!painted.isEmpty()) {
        Terminal* term = isTerminalRoot() ? static_cast<Terminal*>(this) : terminal();
        if (TerminalEngine* eng = term ? term->engineIfInitialized() : nullptr) {
            eng->addDamageRect(painted);
        }
    }
}

void Item::collectDirtyDamage(int absX, int absY, const QRect& ancestorClip, QRect& outDamage,
                              bool& outFullDamage) const {
    if (outFullDamage || !m_d->visible) return;
    if (!(m_d->dirtyFlags & (DirtyFlag::SelfDirty | DirtyFlag::DescendantDirty))) return;

    if (isTerminalRoot() && m_d->dirtyFlags.testFlag(DirtyFlag::SelfDirty)) {
        outFullDamage = true;
        return;
    }

    if (m_d->dirtyFlags.testFlag(DirtyFlag::SelfDirty)) {
        if (!m_d->lastPaintedSubtreeRect.isEmpty()) {
            outDamage |= m_d->lastPaintedSubtreeRect;
        }
        const QRect currentSubtree =
            ensureSubtreeBounds().translated(absX, absY).intersected(ancestorClip);
        if (!currentSubtree.isEmpty()) {
            outDamage |= currentSubtree;
        }
    }

    if (!m_d->dirtyFlags.testFlag(DirtyFlag::DescendantDirty) || m_d->visualChildren.isEmpty()) {
        return;
    }

    QRect childrenClip = ancestorClip;
    if (m_d->clip && !isTerminalRoot()) {
        const ClipRect c =
            clipRectForChildren(absX, absY,
                                ClipRect{ancestorClip.x(), ancestorClip.y(), ancestorClip.width(),
                                         ancestorClip.height()});
        childrenClip = QRect(c.x, c.y, c.width, c.height);
    }

    const QPoint offset = contentOffset();
    for (Item* child : m_d->visualChildren) {
        if (!child || !child->visible() || child->isPopup()) continue;
        if (!(child->m_d->dirtyFlags & (DirtyFlag::SelfDirty | DirtyFlag::DescendantDirty)))
            continue;
        child->collectDirtyDamage(absX + child->x() - offset.x(), absY + child->y() - offset.y(),
                                  childrenClip, outDamage, outFullDamage);
        if (outFullDamage) return;
    }
}

// Top-down layout pass. Runs updateLayout() wherever LayoutDirty is set, then
// recurses into children, and re-runs this item's layout once if a child's
// layout changed this item's implicit size. Iterations are bounded so an
// updateLayout() that keeps invalidating itself cannot hang the frame.
void Item::layoutSubtree() {
    if (!m_d->visible) return;
    if (!(m_d->dirtyFlags & (DirtyFlag::LayoutDirty | DirtyFlag::DescendantLayoutDirty))) return;
    m_d->dirtyFlags &= ~DirtyFlags(DirtyFlag::DescendantLayoutDirty);

    QPointer<Item> selfGuard(this);
    constexpr int MaxLayoutPasses = 3;

    for (int pass = 0; pass < MaxLayoutPasses && m_d->dirtyFlags.testFlag(DirtyFlag::LayoutDirty);
         ++pass) {
        m_d->dirtyFlags &= ~DirtyFlags(DirtyFlag::LayoutDirty);
        m_d->dirtyFlags |= DirtyFlag::SelfDirty;
        invalidateSubtreeBounds();
        updateLayout();
        if (!selfGuard) return;
    }

    const QList<Item*>& orderedChildren = stackingChildren();
    if (orderedChildren.size() == 1) {
        if (Item* childItem = orderedChildren[0]) childItem->layoutSubtree();
        if (!selfGuard) return;
    } else if (orderedChildren.size() > 1) {
        QVarLengthArray<QPointer<Item>, 16> children;
        children.reserve(orderedChildren.size());
        for (Item* child : orderedChildren)
            children.append(child);
        for (const QPointer<Item>& childGuard : children) {
            if (Item* childItem = childGuard.data()) childItem->layoutSubtree();
            if (!selfGuard) return;
        }
    }

    if (m_d->dirtyFlags.testFlag(DirtyFlag::LayoutDirty)) {
        m_d->dirtyFlags &= ~DirtyFlags(DirtyFlag::LayoutDirty);
        m_d->dirtyFlags |= DirtyFlag::SelfDirty;
        invalidateSubtreeBounds();
        updateLayout();
        if (!selfGuard) return;
    }
    m_d->dirtyFlags &= ~DirtyFlags(DirtyFlag::DescendantLayoutDirty);
    (void)ensureSubtreeBounds();
}

void Item::paintSubtree(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_d->visible) return;

    QPointer<Item> selfGuard(this);

    // Layout normally ran in layoutSubtree(); this handles items painted
    // without a preceding layout pass and layouts invalidated by a sibling's
    // paint.
    const DirtyFlags consumedFlags = m_d->dirtyFlags;
    m_d->dirtyFlags &= ~consumedFlags;
    if (consumedFlags.testFlag(DirtyFlag::LayoutDirty)) {
        invalidateSubtreeBounds();
        updateLayout();
        if (!selfGuard) return;
    }

    m_d->lastPaintedSubtreeRect = ensureSubtreeBounds()
                                      .translated(absX, absY)
                                      .intersected(QRect(0, 0, buffer.width(), buffer.height()));

    paint(buffer, absX, absY, clipRect);
    if (!selfGuard) return;

    if (!m_d->visualChildren.isEmpty()) {
        const ClipRect childrenClip = clipRectForChildren(absX, absY, clipRect);
        if (!m_d->clip || !childrenClip.isEmpty()) {
            const ClipRect layoutClip =
                m_d->clip ? clipRectForChildren(absX, absY,
                                                ClipRect{0, 0, buffer.width(), buffer.height()})
                          : ClipRect{0, 0, buffer.width(), buffer.height()};
            const QPoint offset = contentOffset();
            auto clearPaintedRects = [](auto& self, Item* cur) -> void {
                if (!cur) return;
                cur->m_d->lastPaintedSubtreeRect = QRect();
                for (Item* child : cur->m_d->visualChildren) {
                    self(self, child);
                }
            };
            auto visitChild = [&](Item* childItem) {
                if (!childItem) return;
                if (childItem->isPopup() && childItem->visible())
                    return; // painted by Terminal::paintOverlay
                const int childAbsX = absX + childItem->x() - offset.x();
                const int childAbsY = absY + childItem->y() - offset.y();
                if (!(childItem->m_d->dirtyFlags &
                      (DirtyFlag::LayoutDirty | DirtyFlag::DescendantLayoutDirty))) {
                    const QRect childSubtree =
                        childItem->ensureSubtreeBounds().translated(childAbsX, childAbsY);
                    const ClipRect childClipRect{childSubtree.x(), childSubtree.y(),
                                                 childSubtree.width(), childSubtree.height()};
                    if (childClipRect.isEmpty() ||
                        childrenClip.intersected(childClipRect).isEmpty()) {
                        if (childItem->m_d->dirtyFlags &
                            (DirtyFlag::SelfDirty | DirtyFlag::DescendantDirty)) {
                            childItem->clearPaintDirtySubtree();
                        }
                        if (childClipRect.isEmpty() ||
                            layoutClip.intersected(childClipRect).isEmpty()) {
                            if (!childItem->m_d->lastPaintedSubtreeRect.isEmpty()) {
                                clearPaintedRects(clearPaintedRects, childItem);
                            }
                        }
                        return;
                    }
                }
                childItem->paintSubtree(buffer, childAbsX, childAbsY, childrenClip);
            };

            const QList<Item*>& orderedChildren = stackingChildren();
            if (orderedChildren.size() == 1) {
                visitChild(orderedChildren[0]);
                if (!selfGuard) return;
            } else if (orderedChildren.size() > 1) {
                QVarLengthArray<QPointer<Item>, 16> children;
                children.reserve(orderedChildren.size());
                for (Item* child : orderedChildren)
                    children.append(child);
                for (const QPointer<Item>& childGuard : children) {
                    visitChild(childGuard.data());
                    if (!selfGuard) return;
                }
            }
        }
    }

    paintOverlay(buffer, absX, absY, clipRect);
}

void Item::updateLayout() {
    const int lp = leftPadding();
    const int tp = topPadding();
    for (Item* child : m_d->visualChildren) {
        if (!child || !child->visible() || child->isPopup() || child->ignoresParentPadding())
            continue;
        const Anchors* a = child->anchorsIfAny();
        const bool hasHAnchor =
            a && (a->fill() || a->centerIn() || a->left() || a->right() || a->horizontalCenter());
        const bool hasVAnchor =
            a && (a->fill() || a->centerIn() || a->top() || a->bottom() || a->verticalCenter());
        if (!hasHAnchor && !child->hasExplicitX()) {
            child->setDefaultX(lp);
        }
        if (!hasVAnchor && !child->hasExplicitY()) {
            child->setDefaultY(tp);
        }
    }
}

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
    if (!m_d->clip) return clipRect;
    return clipRect.intersected(ClipRect{absX, absY, width(), height()});
}

int Item::width() const {
    return m_d->width >= 0 ? m_d->width : implicitWidth();
}

int Item::height() const {
    return m_d->height >= 0 ? m_d->height : implicitHeight();
}

void Item::setPadding(int p) {
    if (m_d->padding == p) return;
    m_d->padding = p;
    emit paddingChanged();
    if (m_d->leftPadding < 0) emit leftPaddingChanged();
    if (m_d->rightPadding < 0) emit rightPaddingChanged();
    if (m_d->topPadding < 0) emit topPaddingChanged();
    if (m_d->bottomPadding < 0) emit bottomPaddingChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

void Item::setClip(bool c) {
    if (m_d->clip == c) return;
    m_d->clip = c;
    invalidateSubtreeBounds();
    emit clipChanged();
    markDirty();
}

int Item::leftPadding() const {
    return m_d->leftPadding >= 0 ? m_d->leftPadding : m_d->padding;
}

void Item::setLeftPadding(int p) {
    if (m_d->leftPadding == p) return;
    m_d->leftPadding = p;
    emit leftPaddingChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

int Item::rightPadding() const {
    return m_d->rightPadding >= 0 ? m_d->rightPadding : m_d->padding;
}

void Item::setRightPadding(int p) {
    if (m_d->rightPadding == p) return;
    m_d->rightPadding = p;
    emit rightPaddingChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

int Item::topPadding() const {
    return m_d->topPadding >= 0 ? m_d->topPadding : m_d->padding;
}

void Item::setTopPadding(int p) {
    if (m_d->topPadding == p) return;
    m_d->topPadding = p;
    emit topPaddingChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
}

int Item::bottomPadding() const {
    return m_d->bottomPadding >= 0 ? m_d->bottomPadding : m_d->padding;
}

void Item::setBottomPadding(int p) {
    if (m_d->bottomPadding == p) return;
    m_d->bottomPadding = p;
    emit bottomPaddingChanged();
    markDirty(DirtyFlags(DirtyFlag::LayoutDirty) | DirtyFlag::SelfDirty);
    notifyImplicitSizeIfChanged();
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
    if (!m_d->stateController) {
        auto* self = const_cast<Item*>(this);
        m_d->stateController = std::make_unique<StateController>(self);
        m_d->stateController->setDefaultTarget(self);
        connect(m_d->stateController.get(), &StateController::stateChanged, self,
                &Item::stateChanged);
    }
    return m_d->stateController.get();
}

QQmlListProperty<qmltui::State> Item::states() const {
    return ensureStateController()->statesProperty();
}

QQmlListProperty<qmltui::Transition> Item::transitions() const {
    return ensureStateController()->transitionsProperty();
}

QString Item::state() const {
    return m_d->stateController ? m_d->stateController->state() : QString();
}

void Item::setState(const QString& newState) {
    if (!m_d->stateController && newState.isEmpty()) return;
    ensureStateController()->setState(newState);
}

void Item::evaluateStateConditions() {
    if (m_d->stateController) m_d->stateController->evaluateStateConditions();
}

} // namespace qmltui
