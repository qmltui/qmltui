#pragma once

#include <Qamui/Core/items/Item.h>
#include <Qamui/Core/engine/RenderPhase_p.h>

namespace qamui {

// Internal entry point for framework-owned Item tree layout and painting. This
// header is intentionally not installed; custom items implement updateLayout(),
// paint() and optionally paintOverlay() instead of initiating traversal
// themselves.
class ItemPaintTraversal final
{
public:
    // Layout pass only (RenderPhase::Layout).
    static void layoutSubtree(Item *item)
    {
        if (!item)
            return;
        RenderPhase::Scope scope(RenderPhase::Layout);
        item->layoutSubtree();
    }

    // Paint pass only (RenderPhase::Paint). Items with LayoutDirty still set
    // lay out immediately before painting.
    static void paintSubtree(Item *item, TextGridBuffer &buffer,
                             int absX, int absY,
                             const ClipRect &clipRect = ClipRect())
    {
        if (!item)
            return;
        RenderPhase::Scope scope(RenderPhase::Paint);
        item->paintSubtree(buffer, absX, absY, clipRect);
    }

    // One frame: layout pass followed by paint pass. After it returns,
    // RenderPhase::dirtyDuringPaintCount() reports invalidations raised while
    // painting.
    static void renderSubtree(Item *item, TextGridBuffer &buffer,
                              int absX, int absY,
                              const ClipRect &clipRect = ClipRect())
    {
        layoutSubtree(item);
        paintSubtree(item, buffer, absX, absY, clipRect);
    }
};

} // namespace qamui
