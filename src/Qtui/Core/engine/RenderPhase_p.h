#pragma once

#include <QtGlobal>

namespace qtui {

class Item;

// Tracks which phase of a frame is executing so Item::markDirty can decide
// whether an invalidation must schedule a new frame.
//
//  Idle    outside a frame: markDirty emits dirty() and the engine schedules
//          a render.
//  Layout  updateLayout() pass: geometry changes are expected and are painted
//          by the paint pass of the same frame, so no signal is emitted.
//  Paint   paint()/paintOverlay() pass: invalidations are retained on the item
//          but do not schedule a frame; they are counted and, with
//          QTUI_DEBUG_PAINT_DIRTY=1, reported once per item class/name.
//
// Rendering is single-threaded; the state is process-global.
class RenderPhase final
{
public:
    enum Phase { Idle, Layout, Paint };

    static Phase current();

    // Number of markDirty() calls observed since the current or most recent
    // Paint scope began.
    static int dirtyDuringPaintCount();

    static void recordDirtyDuringPaint(const Item *item);

    class Scope final
    {
    public:
        explicit Scope(Phase phase);
        ~Scope();
        Q_DISABLE_COPY_MOVE(Scope)

    private:
        Phase m_previous;
    };
};

} // namespace qtui
