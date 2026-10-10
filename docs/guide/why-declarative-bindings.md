# Why Declarative Bindings

Most terminal UI frameworks — Ratatui, Bubble Tea, FTXUI, or a raw ncurses
loop — are immediate mode: the UI is a function the application re-runs.
Qtui is retained mode with QML property bindings. This page explains what
that difference means in practice and why Qtui makes this trade.

## Immediate mode: the UI is a function you re-run

In an immediate-mode framework, every tick or event walks the entire view
tree and re-describes the whole screen from application state:

```rust
loop {
    let event = poll_event();
    update_state(&mut app, event);           // mutate state
    terminal.draw(|f| render_ui(f, &app));   // re-describe everything
}
```

The framework diffs the new frame against the previous one before writing
to the terminal, so the *output* is efficient. The *description* is not:
`render_ui` re-executes for every widget on every frame, whether or not
anything it depends on changed. The consequences compound as an
application grows:

- **The application owns the invalidation problem.** Nothing reports
  *what* changed, so the safe move is "redraw everything". Skipping work
  requires hand-rolled dirty flags, and every dirty flag is a chance to
  desynchronize state from screen.
- **Derived state is recomputed or manually cached.** A status bar that
  formats `"{sel}/{total} selected"` runs that formatting every frame — or
  the application caches it and must remember to invalidate the cache.
- **Structure and update logic are entangled.** The render function is
  simultaneously the layout description and the update path, so it grows
  into a procedural dispatch as the application grows.
- **Widgets have no identity between frames.** There is no stable object
  to focus, animate, or attach behavior to; focus chains, animation
  state, and scroll positions are all encoded in application state and
  re-derived on every frame.

## Declarative bindings: the UI is a graph you declare once

In Qtui, a QML expression establishes a persistent dependency edge:

```qml
Text {
    text: model.selectedCount + "/" + model.totalCount + " selected"
    color: list.activeFocus ? "cyan" : root.dimColor
}
```

The `text` line is not an assignment executed in a loop — it is a
subscription. The QML engine records that this property depends on
`selectedCount` and `totalCount`. When `selectedCount` changes, exactly
this expression re-evaluates, exactly this item is marked dirty, and the
renderer repaints exactly the affected cells (see
[architecture](../architecture.md) for the frame-diff pipeline).
The application never writes an update path at all:

- **Invalidation is automatic and minimal.** The dependency graph *is*
  the dirty tracking. There is no bug class where state changed but the
  screen did not, and no waste class where unchanged widgets redraw.
- **Update cost scales with the change, not the UI.** A clock updating
  once a second in a 200-item dashboard costs one expression
  re-evaluation and a few cells — not a 200-item tree walk. Over SSH,
  this maps directly onto minimal VT output.
- **Items have identity.** Because the tree is retained, an item persists
  across frames. Focus chains, `Behavior` animations, overlays, and
  virtualized view delegates are framework features rather than patterns
  every application reimplements in its state struct.
- **Structure and logic separate cleanly.** The QML file states what the
  UI *is*; the model states what the data *is*; bindings are the entire
  glue.

## The honest trade

Immediate mode has real advantages: the UI is a pure function of state,
which makes it trivially debuggable — print the state and you know the
frame. There is no hidden engine, no binding loops, no property-lifetime
pitfalls, and terminals are cheap enough to brute-force full redraws for
small applications.

Where immediate mode loses is scale and longevity. As a UI grows,
immediate-mode applications accumulate hand-written caching, dirty flags,
and identity bookkeeping — they gradually reimplement an ad-hoc reactive
system. Qtui's position is that if an application is going to end up with
a reactive dependency system anyway, it should be the QML engine's:
correct by construction, and paired with a mature declarative language.
