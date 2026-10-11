#pragma once

#include <Qamui/Core/engine/ItemPaintTraversal_p.h>

namespace qamui::testing {

// Renders one frame the way the engine does: layout pass, then paint pass.
inline void paintItem(Item &item, TextGridBuffer &buffer,
                      int absX, int absY,
                      const ClipRect &clipRect = ClipRect())
{
    ItemPaintTraversal::renderSubtree(&item, buffer, absX, absY, clipRect);
}

inline void paintItem(Item *item, TextGridBuffer &buffer,
                      int absX, int absY,
                      const ClipRect &clipRect = ClipRect())
{
    ItemPaintTraversal::renderSubtree(item, buffer, absX, absY, clipRect);
}

} // namespace qamui::testing
