# Qtui 0.1 Stabilization Migration Notes

## QML applications

- Control components now report their natural size through `implicitWidth` and
  `implicitHeight`. Explicit consumer-assigned `width` and `height` continue to
  work and are no longer replaced by an internal binding.
- Control sizing uses terminal display cells. CJK, emoji, combining sequences,
  translated labels, dialog buttons, and slider values can therefore produce a
  different and more accurate natural width.
- `Slider.trackWidth` is now readonly because it is derived from the assigned
  control width and value-label width.
- Plain text renders terminal control characters visibly. Select ANSI text
  format explicitly when escape-sequence interpretation is intended.

## Custom C++ items

- `Item::setVisible()` and `Item::setParentItem()` are virtual. Overrides must
  use `override` and should call the base implementation unless they completely
  replace its notification and dirty-state behavior.
- The QML `parent` property is writable and emits `parentChanged`; it is no
  longer declared constant.
- `TextGridBuffer::resize()` returns `bool` and rejects negative, overflowing,
  or excessively large dimensions. Custom renderers should check the result
  when dimensions do not originate from a validated terminal size.
- `TerminalDevice` and renderer implementation APIs are internal and are not
  part of the installed customization headers.

## Build integration

- Use `find_package(Qtui 0.1 REQUIRED)` and link `Qtui::Core` for custom items.
- QML modules install below Qt's standard `<prefix>/lib/qt6/qml` import root.

### Module and target names

- Replace `import Terminal` with `import Qtui`.
- Replace `import Terminal.Controls` with `import Qtui.Controls`.
- Replace `import Terminal.Widgets` with `import Qtui.Widgets`.
- Replace the old build targets with `QtuiCore`, `QtuiControls`, and
  `QtuiWidgets` inside the source tree, or the exported targets `Qtui::Core`,
  `Qtui::Controls`, and `Qtui::Widgets` after `find_package(Qtui)`.
