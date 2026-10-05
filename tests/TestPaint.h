#pragma once

#include "ItemPaintTraversal_p.h"

namespace qtui::testing {

inline void paintItem(Item &item, TextGridBuffer &buffer,
                      int absX, int absY,
                      const ClipRect &clipRect = ClipRect())
{
    ItemPaintTraversal::paintSubtree(&item, buffer, absX, absY, clipRect);
}

inline void paintItem(Item *item, TextGridBuffer &buffer,
                      int absX, int absY,
                      const ClipRect &clipRect = ClipRect())
{
    ItemPaintTraversal::paintSubtree(item, buffer, absX, absY, clipRect);
}

} // namespace qtui::testing
