#pragma once

#include "../items/Item.h"

namespace qtui {

// Internal entry point for framework-owned Item tree painting. This header is
// intentionally not installed; custom items implement paint() and optionally
// paintOverlay() instead of initiating traversal themselves.
class ItemPaintTraversal final
{
public:
    static void paintSubtree(Item *item, TextGridBuffer &buffer,
                             int absX, int absY,
                             const ClipRect &clipRect = ClipRect())
    {
        if (item)
            item->paintSubtree(buffer, absX, absY, clipRect);
    }
};

} // namespace qtui
