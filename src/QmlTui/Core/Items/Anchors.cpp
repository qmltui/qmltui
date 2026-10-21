#include <QmlTui/Core/Items/Anchors.h>
#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Items/AnchorLine.h>

namespace qmltui {

namespace {

struct ResolvingGuard {
    bool& flag;
    bool active;
    ResolvingGuard(bool& f) : flag(f), active(!f) {
        if (active) flag = true;
    }
    ~ResolvingGuard() {
        if (active) flag = false;
    }
};

void connectTarget(Item* target, Anchors* anchors) {
    if (!target) return;
    QObject::connect(target, &Item::xChanged, anchors, &Anchors::resolve, Qt::UniqueConnection);
    QObject::connect(target, &Item::yChanged, anchors, &Anchors::resolve, Qt::UniqueConnection);
    QObject::connect(target, &Item::widthChanged, anchors, &Anchors::resolve, Qt::UniqueConnection);
    QObject::connect(target, &Item::heightChanged, anchors, &Anchors::resolve,
                     Qt::UniqueConnection);
    QObject::connect(target, &Item::implicitWidthChanged, anchors, &Anchors::resolve,
                     Qt::UniqueConnection);
    QObject::connect(target, &Item::implicitHeightChanged, anchors, &Anchors::resolve,
                     Qt::UniqueConnection);
}

bool isReferencedBy(const Anchors* anchors, const Item* target) {
    if (!anchors || !target) return false;
    if (anchors->fill() == target || anchors->centerIn() == target) return true;
    if (anchors->left() && anchors->left()->owner() == target) return true;
    if (anchors->right() && anchors->right()->owner() == target) return true;
    if (anchors->top() && anchors->top()->owner() == target) return true;
    if (anchors->bottom() && anchors->bottom()->owner() == target) return true;
    if (anchors->horizontalCenter() && anchors->horizontalCenter()->owner() == target) return true;
    if (anchors->verticalCenter() && anchors->verticalCenter()->owner() == target) return true;
    return false;
}

void disconnectTarget(Item* target, Anchors* anchors) {
    if (!target || isReferencedBy(anchors, target)) return;
    QObject::disconnect(target, &Item::xChanged, anchors, &Anchors::resolve);
    QObject::disconnect(target, &Item::yChanged, anchors, &Anchors::resolve);
    QObject::disconnect(target, &Item::widthChanged, anchors, &Anchors::resolve);
    QObject::disconnect(target, &Item::heightChanged, anchors, &Anchors::resolve);
    QObject::disconnect(target, &Item::implicitWidthChanged, anchors, &Anchors::resolve);
    QObject::disconnect(target, &Item::implicitHeightChanged, anchors, &Anchors::resolve);
}

} // namespace

Anchors::Anchors(Item* owner) : QObject(owner), m_owner(owner) {}

Anchors::~Anchors() = default;

bool Anchors::hasAnchors() const {
    return m_fillTarget || m_centerInTarget || m_left || m_right || m_top || m_bottom ||
           m_horizontalCenter || m_verticalCenter;
}

void Anchors::ensureOwnerConnections() {
    if (m_ownerConnected || !m_owner) return;
    m_ownerConnected = true;
    connect(m_owner, &Item::widthChanged, this, &Anchors::resolve, Qt::UniqueConnection);
    connect(m_owner, &Item::implicitWidthChanged, this, &Anchors::resolve, Qt::UniqueConnection);
    connect(m_owner, &Item::heightChanged, this, &Anchors::resolve, Qt::UniqueConnection);
    connect(m_owner, &Item::implicitHeightChanged, this, &Anchors::resolve, Qt::UniqueConnection);
    connect(m_owner, &Item::parentChanged, this, &Anchors::resolve, Qt::UniqueConnection);
}

QObject* Anchors::fill() const {
    return m_fillTarget.data();
}

void Anchors::setFill(QObject* target) {
    Item* itemTarget = qobject_cast<Item*>(target);
    if (m_fillTarget.data() == itemTarget) return;

    Item* oldTarget = m_fillTarget.data();
    m_fillTarget = itemTarget;
    if (oldTarget) {
        disconnectTarget(oldTarget, this);
    }

    if (m_fillTarget) {
        connectTarget(m_fillTarget.data(), this);
    }

    emit fillChanged();
    resolve();
}

QObject* Anchors::centerIn() const {
    return m_centerInTarget.data();
}

void Anchors::setCenterIn(QObject* target) {
    Item* itemTarget = qobject_cast<Item*>(target);
    if (m_centerInTarget.data() == itemTarget) return;

    Item* oldTarget = m_centerInTarget.data();
    m_centerInTarget = itemTarget;
    if (oldTarget) {
        disconnectTarget(oldTarget, this);
    }

    if (m_centerInTarget) {
        connectTarget(m_centerInTarget.data(), this);
    }

    emit centerInChanged();
    resolve();
}

AnchorLine* Anchors::left() const {
    return m_left.data();
}
void Anchors::setLeft(AnchorLine* line) {
    if (m_left.data() == line) return;
    Item* oldTarget = m_left ? m_left->owner() : nullptr;
    m_left = line;
    if (oldTarget) {
        disconnectTarget(oldTarget, this);
    }
    if (m_left) {
        connectTarget(m_left->owner(), this);
    }
    emit leftChanged();
    resolve();
}

AnchorLine* Anchors::right() const {
    return m_right.data();
}
void Anchors::setRight(AnchorLine* line) {
    if (m_right.data() == line) return;
    Item* oldTarget = m_right ? m_right->owner() : nullptr;
    m_right = line;
    if (oldTarget) {
        disconnectTarget(oldTarget, this);
    }
    if (m_right) {
        connectTarget(m_right->owner(), this);
    }
    emit rightChanged();
    resolve();
}

AnchorLine* Anchors::top() const {
    return m_top.data();
}
void Anchors::setTop(AnchorLine* line) {
    if (m_top.data() == line) return;
    Item* oldTarget = m_top ? m_top->owner() : nullptr;
    m_top = line;
    if (oldTarget) {
        disconnectTarget(oldTarget, this);
    }
    if (m_top) {
        connectTarget(m_top->owner(), this);
    }
    emit topChanged();
    resolve();
}

AnchorLine* Anchors::bottom() const {
    return m_bottom.data();
}
void Anchors::setBottom(AnchorLine* line) {
    if (m_bottom.data() == line) return;
    Item* oldTarget = m_bottom ? m_bottom->owner() : nullptr;
    m_bottom = line;
    if (oldTarget) {
        disconnectTarget(oldTarget, this);
    }
    if (m_bottom) {
        connectTarget(m_bottom->owner(), this);
    }
    emit bottomChanged();
    resolve();
}

AnchorLine* Anchors::horizontalCenter() const {
    return m_horizontalCenter.data();
}
void Anchors::setHorizontalCenter(AnchorLine* line) {
    if (m_horizontalCenter.data() == line) return;
    Item* oldTarget = m_horizontalCenter ? m_horizontalCenter->owner() : nullptr;
    m_horizontalCenter = line;
    if (oldTarget) {
        disconnectTarget(oldTarget, this);
    }
    if (m_horizontalCenter) {
        connectTarget(m_horizontalCenter->owner(), this);
    }
    emit horizontalCenterChanged();
    resolve();
}

AnchorLine* Anchors::verticalCenter() const {
    return m_verticalCenter.data();
}
void Anchors::setVerticalCenter(AnchorLine* line) {
    if (m_verticalCenter.data() == line) return;
    Item* oldTarget = m_verticalCenter ? m_verticalCenter->owner() : nullptr;
    m_verticalCenter = line;
    if (oldTarget) {
        disconnectTarget(oldTarget, this);
    }
    if (m_verticalCenter) {
        connectTarget(m_verticalCenter->owner(), this);
    }
    emit verticalCenterChanged();
    resolve();
}

void Anchors::setMargins(int m) {
    if (m_margins == m) return;
    m_margins = m;
    emit marginsChanged();
    if (m_leftMargin < 0) emit leftMarginChanged();
    if (m_rightMargin < 0) emit rightMarginChanged();
    if (m_topMargin < 0) emit topMarginChanged();
    if (m_bottomMargin < 0) emit bottomMarginChanged();
    resolve();
}

int Anchors::leftMargin() const {
    return m_leftMargin >= 0 ? m_leftMargin : m_margins;
}

void Anchors::setLeftMargin(int m) {
    if (m_leftMargin == m) return;
    m_leftMargin = m;
    emit leftMarginChanged();
    resolve();
}

int Anchors::rightMargin() const {
    return m_rightMargin >= 0 ? m_rightMargin : m_margins;
}

void Anchors::setRightMargin(int m) {
    if (m_rightMargin == m) return;
    m_rightMargin = m;
    emit rightMarginChanged();
    resolve();
}

int Anchors::topMargin() const {
    return m_topMargin >= 0 ? m_topMargin : m_margins;
}

void Anchors::setTopMargin(int m) {
    if (m_topMargin == m) return;
    m_topMargin = m;
    emit topMarginChanged();
    resolve();
}

int Anchors::bottomMargin() const {
    return m_bottomMargin >= 0 ? m_bottomMargin : m_margins;
}

void Anchors::setBottomMargin(int m) {
    if (m_bottomMargin == m) return;
    m_bottomMargin = m;
    emit bottomMarginChanged();
    resolve();
}

void Anchors::resolve() {
    if (!m_owner) return;
    if (m_resolving) return;
    if (!hasAnchors()) return;
    ensureOwnerConnections();

    ResolvingGuard guard(m_resolving);

    if (m_fillTarget) {
        int targetW = m_fillTarget->width();
        int targetH = m_fillTarget->height();

        int lm = leftMargin();
        int rm = rightMargin();
        int tm = topMargin();
        int bm = bottomMargin();

        int targetX = 0;
        int targetY = 0;
        if (m_fillTarget.data() != m_owner->parentItem()) {
            targetX = m_fillTarget->x();
            targetY = m_fillTarget->y();
        }

        m_owner->setX(targetX + lm);
        m_owner->setY(targetY + tm);
        m_owner->setWidth((std::max)(0, targetW - lm - rm));
        m_owner->setHeight((std::max)(0, targetH - tm - bm));
        return;
    }

    if (m_centerInTarget) {
        int targetW = m_centerInTarget->width();
        int targetH = m_centerInTarget->height();
        int ownerW = m_owner->width();
        int ownerH = m_owner->height();

        int targetX = 0;
        int targetY = 0;
        if (m_centerInTarget.data() != m_owner->parentItem()) {
            targetX = m_centerInTarget->x();
            targetY = m_centerInTarget->y();
        }

        m_owner->setX(targetX + (targetW - ownerW) / 2);
        m_owner->setY(targetY + (targetH - ownerH) / 2);
        return;
    }

    auto isValidTarget = [this](Item* target) -> bool {
        if (!target) return false;
        Item* parent = m_owner->parentItem();
        if (target == parent) return true;
        if (parent && target->parentItem() == parent) return true;
        return false;
    };

    auto getLineCoord = [this, &isValidTarget](AnchorLine* line) -> int {
        if (!line) return 0;
        Item* target = line->owner();
        if (!target) return 0;
        if (!isValidTarget(target)) {
            qWarning() << "Anchors: target item must be the parent or a sibling.";
            return 0;
        }

        if (target == m_owner->parentItem()) {
            switch (line->edge()) {
            case AnchorLine::Left:
                return 0;
            case AnchorLine::Right:
                return target->width();
            case AnchorLine::Top:
                return 0;
            case AnchorLine::Bottom:
                return target->height();
            case AnchorLine::HorizontalCenter:
                return target->width() / 2;
            case AnchorLine::VerticalCenter:
                return target->height() / 2;
            }
        } else {
            switch (line->edge()) {
            case AnchorLine::Left:
                return target->x();
            case AnchorLine::Right:
                return target->x() + target->width();
            case AnchorLine::Top:
                return target->y();
            case AnchorLine::Bottom:
                return target->y() + target->height();
            case AnchorLine::HorizontalCenter:
                return target->x() + target->width() / 2;
            case AnchorLine::VerticalCenter:
                return target->y() + target->height() / 2;
            }
        }
        return 0;
    };

    bool hasLeft = m_left && isValidTarget(m_left->owner());
    bool hasRight = m_right && isValidTarget(m_right->owner());
    bool hasHC = m_horizontalCenter && isValidTarget(m_horizontalCenter->owner());

    if (hasLeft && hasRight) {
        int x1 = getLineCoord(m_left.data()) + leftMargin();
        int x2 = getLineCoord(m_right.data()) - rightMargin();
        m_owner->setX(x1);
        m_owner->setWidth((std::max)(0, x2 - x1));
    } else if (hasHC) {
        int hc = getLineCoord(m_horizontalCenter.data());
        m_owner->setX(hc - m_owner->width() / 2);
    } else if (hasLeft) {
        int x1 = getLineCoord(m_left.data()) + leftMargin();
        m_owner->setX(x1);
    } else if (hasRight) {
        int x2 = getLineCoord(m_right.data()) - rightMargin();
        m_owner->setX(x2 - m_owner->width());
    }

    bool hasTop = m_top && isValidTarget(m_top->owner());
    bool hasBottom = m_bottom && isValidTarget(m_bottom->owner());
    bool hasVC = m_verticalCenter && isValidTarget(m_verticalCenter->owner());

    if (hasTop && hasBottom) {
        int y1 = getLineCoord(m_top.data()) + topMargin();
        int y2 = getLineCoord(m_bottom.data()) - bottomMargin();
        m_owner->setY(y1);
        m_owner->setHeight((std::max)(0, y2 - y1));
    } else if (hasVC) {
        int vc = getLineCoord(m_verticalCenter.data());
        m_owner->setY(vc - m_owner->height() / 2);
    } else if (hasTop) {
        int y1 = getLineCoord(m_top.data()) + topMargin();
        m_owner->setY(y1);
    } else if (hasBottom) {
        int y2 = getLineCoord(m_bottom.data()) - bottomMargin();
        m_owner->setY(y2 - m_owner->height());
    }
}

} // namespace qmltui
