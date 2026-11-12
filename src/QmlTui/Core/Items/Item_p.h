#pragma once

// Internal: private data storage for Item.

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Engine/StateController_p.h>
#include <QList>
#include <QPointer>
#include <QRect>
#include <memory>

namespace qmltui {

struct ItemPrivate {
    int x = 0;
    int y = 0;
    qreal z = 0;
    int width = -1;
    int height = -1;
    int implicitWidth = 0;
    int implicitHeight = 0;
    int reportedImplicitWidth = 0;
    int reportedImplicitHeight = 0;
    bool visible = true;
    bool enabled = true;
    bool focus = false;
    bool activeFocus = false;
    Qt::FocusPolicy focusPolicy = Qt::NoFocus;
    Item* parentItem = nullptr;
    std::unique_ptr<Anchors> anchors;
    AnchorLine* leftLine = nullptr;
    AnchorLine* rightLine = nullptr;
    AnchorLine* topLine = nullptr;
    AnchorLine* bottomLine = nullptr;
    AnchorLine* horizontalCenterLine = nullptr;
    AnchorLine* verticalCenterLine = nullptr;
    bool clip = false;
    int padding = 0;
    int leftPadding = -1;
    int rightPadding = -1;
    int topPadding = -1;
    int bottomPadding = -1;
    QList<QPointer<QObject>> dataChildren;
    QList<Item*> visualChildren;
    QList<Item*> stackingChildren;
    bool stackingOrderDirty = true;
    std::unique_ptr<StateController> stateController;
    bool filtersChildMouseEvents = false;
    quint8 itemFlags = 0;
    Item::DirtyFlags dirtyFlags = Item::DirtyFlag::SelfDirty;
    mutable QRect subtreeBounds;
    mutable bool subtreeBoundsValid = false;
    QRect lastPaintedSubtreeRect;
};

} // namespace qmltui
