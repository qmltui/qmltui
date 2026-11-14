# MouseArea

`MouseArea` is an invisible `Item` that handles terminal mouse presses, releases, clicks, wheel events, and pointer coordinates (`mouseX`, `mouseY`). Button-held drag motion is always tracked while pressed; passive hover motion without a pressed button is enabled via `hoverEnabled`.

- **Header**: `<QmlTui/Core/Items/MouseArea.h>`
- **Inherits**: `qmltui::Item`
- **Target**: `qmltui::core`

## Properties (`Q_PROPERTY`)

| Property | Type | Access | Notify Signal | Description |
| :--- | :--- | :--- | :--- | :--- |
| `mouseX`, `mouseY` | `int` | read-only | `mouseXChanged`, `mouseYChanged` | Pointer coordinates relative to this `MouseArea`. |
| `acceptedButtons` | `Qt::MouseButtons` | read/write | `acceptedButtonsChanged` | Buttons this area reacts to (`Qt::LeftButton` by default); presses with other buttons remain unaccepted. |
| `pressed` | `bool` | read-only | `pressedChanged` | Whether an accepted mouse button is currently held down after pressing this `MouseArea` (C++ getter: `isPressed()`). |
| `pressedButtons` | `Qt::MouseButtons` | read-only | `pressedButtonsChanged` | Currently held accepted mouse buttons on this `MouseArea`. |
| `containsPress` | `bool` | read-only | `containsPressChanged` | `true` while `pressed` is `true` and the pointer remains inside this `MouseArea` (tracks drag out/in without `hoverEnabled`). |
| `hoverEnabled` | `bool` | read/write | `hoverEnabledChanged` | Requests passive mouse-motion reporting while `true` (`false` by default). |
| `containsMouse` | `bool` | read-only | `containsMouseChanged` | Whether the mouse pointer is currently inside this `MouseArea` while `hoverEnabled` is `true`. |

## Public Methods

- `bool isPressed() const` — C++ getter for the `pressed` property (`true` while `pressedButtons() != Qt::NoButton`).
- `void setMousePosition(int x, int y)` — updates `mouseX` and `mouseY` and emits their change signals when modified.
- `void mousePressEvent(MouseEvent *event) override`, `void mouseReleaseEvent(MouseEvent *event) override`, `void mouseMoveEvent(MouseEvent *event) override`, `void wheelEvent(MouseEvent *event) override` — item mouse and wheel event handlers.

## Signals

- `void pressed(qmltui::MouseEvent *event)`
- `void released(qmltui::MouseEvent *event)`
- `void clicked(qmltui::MouseEvent *event)`
- `void wheel(qmltui::MouseEvent *event)`
- `void entered()` / `void exited()`

See the [MouseArea QML type reference](../qml-types/mousearea.md) for QML usage.
