# Qtui 0.3 Migration Notes

Qtui 0.3 intentionally breaks the provisional 0.2 custom-item API; there is no
compatibility shim.

## Custom C++ items

- Replace `render(...) override` with protected `paint(...) override`. Do not
  call `Item::render()` or render children manually; traversal is framework-owned.
- Move deferred geometry work into `updateLayout()` and call `markDirty()` when
  a property affects either painting or layout.
- Replace public `QString`/`QColor` color properties with `qtui::Color`. Use
  `Color::argb32()` only at terminal-cell or native-library boundaries.
- Use `TextGridBuffer::tryCellAt()` for coordinates that can be clipped or out
  of bounds. `cellAt()` now asserts that coordinates are valid.
- `MouseEvent::type()` distinguishes press, release, move, and wheel events.
  The obsolete `Item::keyReleaseEvent()`, `Keys.released`,
  `Item::setGeometryDirect()`, and `Item::removeChildItem()` APIs are removed.

## Build integration

Request `find_package(Qtui 0.3 REQUIRED)` and link custom items to `Qtui::Core`.
The package uses same-minor compatibility during the 0.x stabilization series.
