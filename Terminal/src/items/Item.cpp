#include "Item.h"
#include "AnchorLine.h"
#include "../engine/Keys.h"
#include "../engine/KeyEvent.h"
#include "../engine/MouseEvent.h"
#include "Terminal.h"
#include "../core/ColorMapper.h"
#include "../engine/State.h"
#include "../engine/Transition.h"
#include "../engine/Animation.h"
#include <QSet>
#include <QChildEvent>
#include <QPointer>

namespace qtui {

Item::Item(QObject *parent) : QObject(parent), m_anchors(this) {
    m_leftLine = new AnchorLine(AnchorLine::Left, this);
    m_rightLine = new AnchorLine(AnchorLine::Right, this);
    m_topLine = new AnchorLine(AnchorLine::Top, this);
    m_bottomLine = new AnchorLine(AnchorLine::Bottom, this);
    m_horizontalCenterLine = new AnchorLine(AnchorLine::HorizontalCenter, this);
    m_verticalCenterLine = new AnchorLine(AnchorLine::VerticalCenter, this);

    if (parent) {
        if (Item* parentItem = qobject_cast<Item*>(parent)) {
            if (!parentItem->m_visualChildren.contains(this)) {
                parentItem->m_visualChildren.append(this);
                parentItem->markDirty();
            }
        }
    }
}

Item::~Item() {
    Terminal* term = terminal();
    if (term && term->focusedItem() == this) {
        term->setFocusedItem(nullptr);
    }
}

QQmlListProperty<QObject> Item::data() const {
    return QQmlListProperty<QObject>(const_cast<Item*>(this), nullptr,
        &Item::appendData,
        &Item::dataCount,
        &Item::dataAt,
        &Item::clearData);
}

void Item::appendData(QQmlListProperty<QObject> *list, QObject *obj) {
    Item *self = qobject_cast<Item*>(list->object);
    if (self && obj) {
        obj->setParent(self);
        self->m_dataChildren.append(obj);
        if (Item* item = qobject_cast<Item*>(obj)) {
            if (!self->m_visualChildren.contains(item)) {
                self->m_visualChildren.append(item);
                self->markDirty();
            }
        }
    }
}

qsizetype Item::dataCount(QQmlListProperty<QObject> *list) {
    Item *self = qobject_cast<Item*>(list->object);
    return self ? self->m_dataChildren.count() : 0;
}

QObject *Item::dataAt(QQmlListProperty<QObject> *list, qsizetype index) {
    Item *self = qobject_cast<Item*>(list->object);
    return self ? self->m_dataChildren.at(index) : nullptr;
}

void Item::clearData(QQmlListProperty<QObject> *list) {
    Item *self = qobject_cast<Item*>(list->object);
    if (self) {
        self->m_dataChildren.clear();
        self->m_visualChildren.clear();
        self->markDirty();
    }
}

void Item::setParentItem(Item *parentItem) {
    if (this->parentItem() == parentItem) return;
    setParent(parentItem);
    emit parentChanged();
    markDirty();
}

void Item::removeChildItem(Item *child) {
    // QObject parenting change handles this via childEvent, but we keep this method
    // in case external callers explicitly call it.
    if (m_visualChildren.removeOne(child)) {
        markDirty();
    }
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

void Item::setWidth(int width) {
    if (m_width == width) return;
    m_width = width;
    emit widthChanged();
    markDirty();
}

void Item::setHeight(int height) {
    if (m_height == height) return;
    m_height = height;
    emit heightChanged();
    markDirty();
}

void Item::setImplicitWidth(int w) {
    if (m_implicitWidth == w) return;
    m_implicitWidth = w;
    emit implicitWidthChanged();
    if (m_width < 0) {
        emit widthChanged();
        markDirty();
    }
}

void Item::setImplicitHeight(int h) {
    if (m_implicitHeight == h) return;
    m_implicitHeight = h;
    emit implicitHeightChanged();
    if (m_height < 0) {
        emit heightChanged();
        markDirty();
    }
}

void Item::setVisible(bool visible) {
    if (m_visible == visible) return;
    m_visible = visible;
    emit visibleChanged();
    if (!visible) {
        Terminal *term = terminal();
        if (term) {
            for (Item *current = term->focusedItem(); current; current = current->parentItem()) {
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
        Terminal *term = terminal();
        if (term) {
            for (Item *current = term->focusedItem(); current; current = current->parentItem()) {
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
    for (const Item *current = this; current; current = current->parentItem()) {
        if (!current->enabled())
            return false;
    }
    return true;
}

void Item::markDirty() {
    emit dirty();
    // Bubble up to parent
    Item* p = qobject_cast<Item*>(parent());
    if (p) {
        p->markDirty();
    }
}

void Item::setFocus(bool focus) {
    if (m_focus == focus) {
        Terminal* term = terminal();
        if (term && focus && term->focusedItem() != this) {
            term->setFocusedItem(this);
        }
        return;
    }
    m_focus = focus;
    emit focusChanged();
    
    Terminal* term = terminal();
    if (term) {
        if (m_focus) {
            term->setFocusedItem(this);
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

void Item::setFocusable(bool focusable) {
    if (m_focusable == focusable) return;
    m_focusable = focusable;
    emit focusableChanged();
    Terminal *term = terminal();
    if (!focusable && term && term->focusedItem() == this)
        term->setFocusedItem(nullptr);
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

QPoint Item::absolutePosition() const {
    int ax = m_x;
    int ay = m_y;
    
    Item* p = qobject_cast<Item*>(parent());
    while (p) {
        // If parent is a Terminal element, we do NOT add its x and y,
        // because Terminal is the screen root. Its x/y is 0 relative to screen.
        if (p->inherits("qtui::Terminal")) {
            break;
        }
        
        ax += p->x();
        ay += p->y();
        
        // Subtract scroll offsets if parent is scrollable
        QVariant scrollY = p->property("contentY");
        if (scrollY.isValid()) {
            ay -= scrollY.toInt();
        }
        QVariant scrollX = p->property("contentX");
        if (scrollX.isValid()) {
            ax -= scrollX.toInt();
        }
        
        p = qobject_cast<Item*>(p->parent());
    }
    return QPoint(ax, ay);
}

void Item::setGeometryDirect(int x, int y, int w, int h) {
    m_x = x;
    m_y = y;
    m_width = w;
    m_height = h;
}

void Item::childEvent(QChildEvent* event) {
    QObject::childEvent(event);
    if (event->added()) {
        if (Item* childItem = qobject_cast<Item*>(event->child())) {
            if (!m_visualChildren.contains(childItem)) {
                m_visualChildren.append(childItem);
                auto childGeometryChanged = [this]() {
                    scheduleChildGeometryInvalidation();
                };
                QList<QMetaObject::Connection> connections;
                connections.append(connect(childItem, &Item::widthChanged, this, childGeometryChanged));
                connections.append(connect(childItem, &Item::heightChanged, this, childGeometryChanged));
                connections.append(connect(childItem, &Item::implicitWidthChanged, this, childGeometryChanged));
                connections.append(connect(childItem, &Item::implicitHeightChanged, this, childGeometryChanged));
                connections.append(connect(childItem, &Item::visibleChanged, this, childGeometryChanged));
                m_childGeometryConnections.insert(childItem, connections);
                markDirty();
            }
            if (childItem->focus()) {
                childItem->setFocus(true);
            }
        }
    } else if (event->removed()) {
        if (Item* childItem = qobject_cast<Item*>(event->child())) {
            const auto connections = m_childGeometryConnections.take(childItem);
            for (const QMetaObject::Connection &connection : connections)
                disconnect(connection);
            m_visualChildren.removeOne(childItem);
            markDirty();
        }
    }
}

void Item::scheduleChildGeometryInvalidation() {
    if (m_childGeometryInvalidationPending) return;
    m_childGeometryInvalidationPending = true;
    QPointer<Item> self(this);
    QMetaObject::invokeMethod(this, [self]() {
        if (!self) return;
        self->m_childGeometryInvalidationPending = false;
        emit self->implicitWidthChanged();
        emit self->implicitHeightChanged();
        self->markDirty();
    }, Qt::QueuedConnection);
}
void Item::render(TextGridBuffer& buffer, int absX, int absY, const ClipRect& clipRect) {
    if (!m_visible) return;
    
    // Resolve anchors first
    m_anchors.resolve();

    int w = width();
    int h = height();

    // Determine the clipping rectangle for children
    ClipRect childrenClip = clipRect;
    if (m_clip) {
        childrenClip = clipRect.intersected(ClipRect{absX, absY, w, h});
    }

    // Render children back-to-front (Painter's Algorithm)
    for (Item* childItem : m_visualChildren) {
        if (childItem) {
            childItem->render(buffer, absX + childItem->x(), absY + childItem->y(), childrenClip);
        }
    }
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

void Item::keyReleaseEvent(class KeyEvent* event) {
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

QQmlListProperty<qtui::State> Item::states() const {
    return QQmlListProperty<qtui::State>(const_cast<Item*>(this), nullptr,
        &Item::appendState,
        &Item::stateCount,
        &Item::stateAt,
        &Item::clearStates);
}

void Item::appendState(QQmlListProperty<qtui::State>* list, qtui::State* state) {
    Item* self = qobject_cast<Item*>(list->object);
    if (self && state) {
        state->setParent(self);
        self->m_statesList.append(state);
        connect(state, &State::whenChanged, self, &Item::evaluateStateConditions);
        self->evaluateStateConditions();
    }
}

qsizetype Item::stateCount(QQmlListProperty<qtui::State>* list) {
    Item* self = qobject_cast<Item*>(list->object);
    return self ? self->m_statesList.size() : 0;
}

qtui::State* Item::stateAt(QQmlListProperty<qtui::State>* list, qsizetype index) {
    Item* self = qobject_cast<Item*>(list->object);
    return self ? self->m_statesList.at(index) : nullptr;
}

void Item::clearStates(QQmlListProperty<qtui::State>* list) {
    Item* self = qobject_cast<Item*>(list->object);
    if (self) {
        self->m_statesList.clear();
    }
}

QQmlListProperty<qtui::Transition> Item::transitions() const {
    return QQmlListProperty<qtui::Transition>(const_cast<Item*>(this), nullptr,
        &Item::appendTransition,
        &Item::transitionCount,
        &Item::transitionAt,
        &Item::clearTransitions);
}

void Item::appendTransition(QQmlListProperty<qtui::Transition>* list, qtui::Transition* transition) {
    Item* self = qobject_cast<Item*>(list->object);
    if (self && transition) {
        transition->setParent(self);
        self->m_transitionsList.append(transition);
    }
}

qsizetype Item::transitionCount(QQmlListProperty<qtui::Transition>* list) {
    Item* self = qobject_cast<Item*>(list->object);
    return self ? self->m_transitionsList.count() : 0;
}

qtui::Transition* Item::transitionAt(QQmlListProperty<qtui::Transition>* list, qsizetype index) {
    Item* self = qobject_cast<Item*>(list->object);
    return self ? self->m_transitionsList.at(index) : nullptr;
}

void Item::clearTransitions(QQmlListProperty<qtui::Transition>* list) {
    Item* self = qobject_cast<Item*>(list->object);
    if (self) {
        self->m_transitionsList.clear();
    }
}

void Item::evaluateStateConditions() {
    State* activeState = nullptr;
    for (State* s : m_statesList) {
        if (s && s->when()) {
            activeState = s;
            break;
        }
    }
    if (activeState) {
        setState(activeState->name());
    } else {
        State* currentObj = nullptr;
        for (State* s : m_statesList) {
            if (s && s->name() == m_stateName) {
                currentObj = s;
                break;
            }
        }
        if (currentObj && currentObj->when() == false) {
            setState(QString());
        }
    }
}

void Item::setState(const QString& newState) {
    if (m_stateName == newState) return;

    QString oldState = m_stateName;
    m_stateName = newState;
    emit stateChanged();

    // 1. Stop any currently running transition animations
    for (const QPointer<Animation>& anim : m_runningTransitionAnimations) {
        if (anim) {
            anim->stop();
            delete anim.data();
        }
    }
    m_runningTransitionAnimations.clear();

    // 2. Find State objects for old and new states
    State* oldStateObj = nullptr;
    State* newStateObj = nullptr;
    for (State* s : m_statesList) {
        if (s) {
            if (s->name() == oldState) oldStateObj = s;
            if (s->name() == newState) newStateObj = s;
        }
    }

    // 3. Find if there is a matching Transition
    Transition* transitionObj = nullptr;
    for (Transition* t : m_transitionsList) {
        if (t && t->matches(oldState, newState)) {
            transitionObj = t;
            break;
        }
    }

    // 4. Compute the set of all modified properties
    struct PropChangeInfo {
        QObject* target;
        QString property;
        QVariant fromVal;
        QVariant toVal;
    };
    QList<PropChangeInfo> modifiedProperties;

    // Helper to find if a property is overridden in a State
    auto findOverrideInState = [](State* state, QObject* defaultTarget, QObject* targetObj, const QString& propName, QVariant& valOut) -> bool {
        if (!state) return false;
        for (QObject* changeObj : state->changesList()) {
            PropertyChanges* pc = qobject_cast<PropertyChanges*>(changeObj);
            if (!pc) continue;
            QObject* pcTarget = pc->target() ? pc->target() : defaultTarget;
            if (pcTarget == targetObj) {
                QVariantMap props = pc->properties();
                if (props.contains(propName)) {
                    valOut = props.value(propName);
                    return true;
                }
            }
        }
        return false;
    };

    // First, scan for properties to revert from the old state
    if (oldStateObj) {
        for (QObject* changeObj : oldStateObj->changesList()) {
            PropertyChanges* pc = qobject_cast<PropertyChanges*>(changeObj);
            if (!pc) continue;
            QObject* oldTarget = pc->target() ? pc->target() : this;
            if (!oldTarget) continue;

            if (pc->restoreEntryValues()) {
                QVariantMap props = pc->properties();
                for (auto it = props.begin(); it != props.end(); ++it) {
                    QString propName = it.key();
                    // Check if the new state overrides it
                    QVariant dummy;
                    if (!findOverrideInState(newStateObj, this, oldTarget, propName, dummy)) {
                        // Needs to be reverted!
                        auto key = qMakePair(QPointer<QObject>(oldTarget), propName);
                        if (m_savedPropertyValues.contains(key)) {
                            PropChangeInfo info;
                            info.target = oldTarget;
                            info.property = propName;
                            info.fromVal = oldTarget->property(propName.toUtf8().constData());
                            info.toVal = m_savedPropertyValues.value(key);
                            modifiedProperties.append(info);
                        }
                    }
                }
            }
        }
    }

    // Next, scan for properties set by the new state
    if (newStateObj) {
        for (QObject* changeObj : newStateObj->changesList()) {
            PropertyChanges* pc = qobject_cast<PropertyChanges*>(changeObj);
            if (!pc) continue;
            QObject* newTarget = pc->target() ? pc->target() : this;
            if (!newTarget) continue;

            QVariantMap props = pc->properties();
            for (auto it = props.begin(); it != props.end(); ++it) {
                QString propName = it.key();
                QVariant newVal = it.value();

                // Save current value if not already saved (so we can revert to it later)
                auto key = qMakePair(QPointer<QObject>(newTarget), propName);
                if (!m_savedPropertyValues.contains(key)) {
                    m_savedPropertyValues.insert(key, newTarget->property(propName.toUtf8().constData()));
                }

                PropChangeInfo info;
                info.target = newTarget;
                info.property = propName;
                info.fromVal = newTarget->property(propName.toUtf8().constData());
                info.toVal = newVal;
                modifiedProperties.append(info);
            }
        }
    }

    // 5. Apply changes, either via Transition animations or immediately
    QSet<QPair<QObject*, QString>> animatedProperties;

    if (transitionObj && !modifiedProperties.isEmpty()) {
        for (Animation* anim : transitionObj->animationsList()) {
            PropertyAnimation* propAnim = qobject_cast<PropertyAnimation*>(anim);
            if (!propAnim) continue;

            // For this animation template, find all matching modified properties
            for (const PropChangeInfo& info : modifiedProperties) {
                // Match target
                if (propAnim->target() && propAnim->target() != info.target) continue;
                // Match property
                if (!propAnim->property().isEmpty() && propAnim->property() != info.property) continue;

                // Match animation type
                bool isColorAnim = (qobject_cast<ColorAnimation*>(propAnim) != nullptr);
                bool isNumberAnim = (qobject_cast<NumberAnimation*>(propAnim) != nullptr);

                // Check type compatibility
                if (isColorAnim) {
                    bool looksLikeColor = info.property.contains("color", Qt::CaseInsensitive) ||
                                          (info.toVal.userType() == QMetaType::QString && info.toVal.toString().startsWith('#'));
                    if (!looksLikeColor) continue;
                } else if (isNumberAnim) {
                    bool isNumeric = (info.toVal.userType() == QMetaType::Int ||
                                      info.toVal.userType() == QMetaType::Double ||
                                      info.toVal.userType() == QMetaType::Float ||
                                      info.toVal.userType() == QMetaType::LongLong ||
                                      info.toVal.userType() == QMetaType::ULongLong);
                    if (!isNumeric) continue;
                }

                // Instantiate running animation copy
                PropertyAnimation* newAnim = nullptr;
                if (isColorAnim) {
                    newAnim = new ColorAnimation(this);
                } else if (isNumberAnim) {
                    newAnim = new NumberAnimation(this);
                } else {
                    newAnim = new PropertyAnimation(this);
                }

                newAnim->setDuration(propAnim->duration());
                newAnim->setEasing(propAnim->easing());
                newAnim->setLoops(propAnim->loops());
                newAnim->setTarget(info.target);
                newAnim->setProperty(info.property);
                newAnim->setFrom(info.fromVal);
                newAnim->setTo(info.toVal);

                // Clean up when done
                connect(newAnim, &Animation::finished, newAnim, [this, newAnim]() {
                    m_runningTransitionAnimations.removeOne(newAnim);
                    newAnim->deleteLater();
                });

                // Track running transition animations
                m_runningTransitionAnimations.append(newAnim);
                animatedProperties.insert(qMakePair(info.target, info.property));

                newAnim->start();
            }
        }
    }

    // Apply any properties that were NOT animated
    for (const PropChangeInfo& info : modifiedProperties) {
        if (!animatedProperties.contains(qMakePair(info.target, info.property))) {
            info.target->setProperty(info.property.toUtf8().constData(), info.toVal);
        }
    }

    // Clean up reverted properties from the saved values map if they are not in the new state
    if (oldStateObj) {
        for (QObject* changeObj : oldStateObj->changesList()) {
            PropertyChanges* pc = qobject_cast<PropertyChanges*>(changeObj);
            if (!pc) continue;
            QObject* oldTarget = pc->target() ? pc->target() : this;
            if (!oldTarget) continue;

            QVariantMap props = pc->properties();
            for (auto it = props.begin(); it != props.end(); ++it) {
                QString propName = it.key();
                QVariant dummy;
                if (!findOverrideInState(newStateObj, this, oldTarget, propName, dummy)) {
                    auto key = qMakePair(QPointer<QObject>(oldTarget), propName);
                    m_savedPropertyValues.remove(key);
                }
            }
        }
    }

    // Prune any dead QPointer entries from m_savedPropertyValues map
    for (auto it = m_savedPropertyValues.begin(); it != m_savedPropertyValues.end(); ) {
        if (it.key().first.isNull()) {
            it = m_savedPropertyValues.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace qtui
