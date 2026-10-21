# MouseArea

`MouseArea` is an invisible `Item` that handles terminal mouse presses, releases, clicks, double-clicks, wheel events, and pointer coordinates (`mouseX`, `mouseY`). Button-held drag motion is always tracked while pressed; passive hover motion without a pressed button is enabled via `hoverEnabled`.

- **Header**: `src/QmlTui/Core/Items/MouseArea.h` *(in-tree item header; not installed as a public header)*
- **Inherits**: `qmltui::Item`

## Properties (`Q_PROPERTY`)

| Property | Type | Access | Notify Signal | Description |
| :--- | :--- | :--- | :--- | :--- |
| `mouseX`, `mouseY` | `int` | read-only | `mouseXChanged`, `mouseYChanged` | Pointer coordinates relative to this `MouseArea`. |
| `hoverEnabled` | `bool` | read/write | `hoverEnabledChanged` | Requests passive mouse-motion reporting while `true` (`false` by default). |
| `containsMouse` | `bool` | read-only | `containsMouseChanged` | Whether the mouse pointer is currently inside this `MouseArea`. |
| `pressed` | `bool` | read-only | `pressedChanged` | Whether a mouse button is currently held down on this `MouseArea`. |

## Signals

- `void pressed(qmltui::MouseEvent *event)`
- `void released(qmltui::MouseEvent *event)`
- `void clicked(qmltui::MouseEvent *event)`
- `void doubleClicked(qmltui::MouseEvent *event)`
- `void positionChanged(qmltui::MouseEvent *event)`
- `void wheel(qmltui::MouseEvent *event)`
- `void entered()` / `void exited()`

See the [MouseArea QML type reference](../qml-types/mousearea.md) for QML usage.
