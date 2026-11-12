#pragma once

// Internal: entry point for framework-owned Item tree layout and painting.

#include <QmlTui/Core/Items/Item.h>
#include <QmlTui/Core/Engine/RenderPhase_p.h>

namespace qmltui {

// Initiates layout (RenderPhase::Layout) and paint (RenderPhase::Paint)
// traversal on an Item subtree; custom items implement updateLayout(), paint(),
// and paintOverlay() instead of calling these directly.
class ItemPaintTraversal final {
public:
    // Layout pass only (RenderPhase::Layout).
    static void layoutSubtree(Item* item) {
        if (!item) return;
        RenderPhase::Scope scope(RenderPhase::Layout);
        item->layoutSubtree();
    }

    // Paint pass only (RenderPhase::Paint). Items with LayoutDirty still set
    // lay out immediately before painting.
    static void paintSubtree(Item* item, TextGridBuffer& buffer, int absX, int absY,
                             const ClipRect& clipRect = ClipRect()) {
        if (!item) return;
        RenderPhase::Scope scope(RenderPhase::Paint);
        item->paintSubtree(buffer, absX, absY, clipRect);
    }

    // One frame: layout pass followed by paint pass. After it returns,
    // RenderPhase::dirtyDuringPaintCount() reports invalidations raised while
    // painting.
    static void renderSubtree(Item* item, TextGridBuffer& buffer, int absX, int absY,
                              const ClipRect& clipRect = ClipRect()) {
        layoutSubtree(item);
        paintSubtree(item, buffer, absX, absY, clipRect);
    }

    // Collects localized screen-space damage from dirty items in a laid-out
    // subtree without mutating dirty flags.
    static void collectDirtyDamage(const Item* item, const QRect& viewportRect, QRect& outDamage,
                                   bool& outFullDamage) {
        if (!item) return;
        item->collectDirtyDamage(0, 0, viewportRect, outDamage, outFullDamage);
    }

    // Clears SelfDirty/DescendantDirty across a subtree when an offscreen-only
    // dirty frame skips painting.
    static void clearPaintDirty(Item* item) {
        if (!item) return;
        RenderPhase::Scope scope(RenderPhase::Paint);
        item->clearPaintDirtySubtree();
    }
};

} // namespace qmltui
