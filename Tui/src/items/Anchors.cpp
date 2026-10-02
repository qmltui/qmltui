#include "Anchors.h"
#include "Item.h"
#include "AnchorLine.h"
#include <QDebug>

namespace qtui {

Anchors::Anchors(Item* owner) : QObject(owner), m_owner(owner) {
}

Anchors::~Anchors() = default;

QObject* Anchors::fill() const {
    return m_fillTarget.data();
}

void Anchors::setFill(QObject* target) {
    Item* itemTarget = qobject_cast<Item*>(target);
    if (m_fillTarget.data() == itemTarget) return;

    if (m_fillTarget) {
        disconnect(m_fillTarget.data(), &Item::widthChanged, this, &Anchors::resolve);
        disconnect(m_fillTarget.data(), &Item::heightChanged, this, &Anchors::resolve);
    }

    m_fillTarget = itemTarget;

    if (m_fillTarget) {
        connect(m_fillTarget.data(), &Item::widthChanged, this, &Anchors::resolve);
        connect(m_fillTarget.data(), &Item::heightChanged, this, &Anchors::resolve);
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

    if (m_centerInTarget) {
        disconnect(m_centerInTarget.data(), &Item::widthChanged, this, &Anchors::resolve);
        disconnect(m_centerInTarget.data(), &Item::heightChanged, this, &Anchors::resolve);
    }

    m_centerInTarget = itemTarget;

    if (m_centerInTarget) {
        connect(m_centerInTarget.data(), &Item::widthChanged, this, &Anchors::resolve);
        connect(m_centerInTarget.data(), &Item::heightChanged, this, &Anchors::resolve);
    }

    emit centerInChanged();
    resolve();
}

AnchorLine* Anchors::left() const { return m_left.data(); }
void Anchors::setLeft(AnchorLine* line) {
    if (m_left.data() == line) return;
    if (m_left) {
        disconnect(m_left->owner(), &Item::xChanged, this, &Anchors::resolve);
        disconnect(m_left->owner(), &Item::widthChanged, this, &Anchors::resolve);
    }
    m_left = line;
    if (m_left) {
        connect(m_left->owner(), &Item::xChanged, this, &Anchors::resolve);
        connect(m_left->owner(), &Item::widthChanged, this, &Anchors::resolve);
    }
    emit leftChanged();
    resolve();
}

AnchorLine* Anchors::right() const { return m_right.data(); }
void Anchors::setRight(AnchorLine* line) {
    if (m_right.data() == line) return;
    if (m_right) {
        disconnect(m_right->owner(), &Item::xChanged, this, &Anchors::resolve);
        disconnect(m_right->owner(), &Item::widthChanged, this, &Anchors::resolve);
    }
    m_right = line;
    if (m_right) {
        connect(m_right->owner(), &Item::xChanged, this, &Anchors::resolve);
        connect(m_right->owner(), &Item::widthChanged, this, &Anchors::resolve);
    }
    emit rightChanged();
    resolve();
}

AnchorLine* Anchors::top() const { return m_top.data(); }
void Anchors::setTop(AnchorLine* line) {
    if (m_top.data() == line) return;
    if (m_top) {
        disconnect(m_top->owner(), &Item::yChanged, this, &Anchors::resolve);
        disconnect(m_top->owner(), &Item::heightChanged, this, &Anchors::resolve);
    }
    m_top = line;
    if (m_top) {
        connect(m_top->owner(), &Item::yChanged, this, &Anchors::resolve);
        connect(m_top->owner(), &Item::heightChanged, this, &Anchors::resolve);
    }
    emit topChanged();
    resolve();
}

AnchorLine* Anchors::bottom() const { return m_bottom.data(); }
void Anchors::setBottom(AnchorLine* line) {
    if (m_bottom.data() == line) return;
    if (m_bottom) {
        disconnect(m_bottom->owner(), &Item::yChanged, this, &Anchors::resolve);
        disconnect(m_bottom->owner(), &Item::heightChanged, this, &Anchors::resolve);
    }
    m_bottom = line;
    if (m_bottom) {
        connect(m_bottom->owner(), &Item::yChanged, this, &Anchors::resolve);
        connect(m_bottom->owner(), &Item::heightChanged, this, &Anchors::resolve);
    }
    emit bottomChanged();
    resolve();
}

AnchorLine* Anchors::horizontalCenter() const { return m_horizontalCenter.data(); }
void Anchors::setHorizontalCenter(AnchorLine* line) {
    if (m_horizontalCenter.data() == line) return;
    if (m_horizontalCenter) {
        disconnect(m_horizontalCenter->owner(), &Item::xChanged, this, &Anchors::resolve);
        disconnect(m_horizontalCenter->owner(), &Item::widthChanged, this, &Anchors::resolve);
    }
    m_horizontalCenter = line;
    if (m_horizontalCenter) {
        connect(m_horizontalCenter->owner(), &Item::xChanged, this, &Anchors::resolve);
        connect(m_horizontalCenter->owner(), &Item::widthChanged, this, &Anchors::resolve);
    }
    emit horizontalCenterChanged();
    resolve();
}

AnchorLine* Anchors::verticalCenter() const { return m_verticalCenter.data(); }
void Anchors::setVerticalCenter(AnchorLine* line) {
    if (m_verticalCenter.data() == line) return;
    if (m_verticalCenter) {
        disconnect(m_verticalCenter->owner(), &Item::yChanged, this, &Anchors::resolve);
        disconnect(m_verticalCenter->owner(), &Item::heightChanged, this, &Anchors::resolve);
    }
    m_verticalCenter = line;
    if (m_verticalCenter) {
        connect(m_verticalCenter->owner(), &Item::yChanged, this, &Anchors::resolve);
        connect(m_verticalCenter->owner(), &Item::heightChanged, this, &Anchors::resolve);
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

    m_resolving = true;

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
        m_owner->setWidth(targetW - lm - rm);
        m_owner->setHeight(targetH - tm - bm);
        
        m_resolving = false;
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
        
        m_resolving = false;
        return;
    }

    auto isValidTarget = [this](Item* target) -> bool {
        if (!target) return false;
        if (target == m_owner->parentItem()) return true;
        Item* parent = m_owner->parentItem();
        if (parent && parent->childItems().contains(target)) return true;
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
                case AnchorLine::Left: return 0;
                case AnchorLine::Right: return target->width();
                case AnchorLine::Top: return 0;
                case AnchorLine::Bottom: return target->height();
                case AnchorLine::HorizontalCenter: return target->width() / 2;
                case AnchorLine::VerticalCenter: return target->height() / 2;
            }
        } else {
            switch (line->edge()) {
                case AnchorLine::Left: return target->x();
                case AnchorLine::Right: return target->x() + target->width();
                case AnchorLine::Top: return target->y();
                case AnchorLine::Bottom: return target->y() + target->height();
                case AnchorLine::HorizontalCenter: return target->x() + target->width() / 2;
                case AnchorLine::VerticalCenter: return target->y() + target->height() / 2;
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
        m_owner->setWidth(std::max(0, x2 - x1));
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
        m_owner->setHeight(std::max(0, y2 - y1));
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

    m_resolving = false;
}

} // namespace qtui
