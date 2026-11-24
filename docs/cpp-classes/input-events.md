# KeyEvent, MouseEvent, Shortcut, and InputScope

These headers define the keyboard, mouse, global shortcut, and modal input-scope contracts dispatched by `InputRouter` to C++ [`Item`](item.md) subclasses and QML handlers.

- **Headers**:
  - `<QmlTui/Core/Engine/KeyEvent.h>`
  - `<QmlTui/Core/Engine/MouseEvent.h>`
  - `<QmlTui/Core/Engine/Shortcut.h>`
  - `<QmlTui/Core/Engine/InputScope.h>`
- **Target**: `QmlTui::Core`

## `qmltui::KeyEvent`

Delivered to `Item::keyPressEvent(KeyEvent*)` and QML [`Keys`](../qml-types/keys.md) attached handlers. Call `setAccepted(true)` to stop propagation up the focus chain and suppress built-in `Tab` focus navigation.

| Method | Return Type | Description |
| :--- | :--- | :--- |
| `key()` | `int` | `Qt::Key` code (for example `Qt::Key_Return`, `Qt::Key_Escape`, `Qt::Key_Up`). |
| `text()` | `QString` | Decoded Unicode text produced by the key press or bracketed paste. |
| `modifiers()` | `int` | Bitmask of `Qt::KeyboardModifiers` (`Qt::ShiftModifier`, `Qt::ControlModifier`, `Qt::AltModifier`). |
| `isPaste()` | `bool` | `true` when `text()` arrived inside a bracketed-paste sequence (`?2004h`). |
| `isAutoRepeat()` | `bool` | `true` when the key press is a terminal-reported auto-repeat (Kitty keyboard event-type `2`). |
| `isAccepted()` / `setAccepted(bool)` | `bool` / `void` | Reads or marks whether the event has been consumed. |

## `qmltui::MouseEvent`

Delivered to `Item::mousePressEvent()`, `mouseReleaseEvent()`, `mouseMoveEvent()`, `wheelEvent()`, and [`MouseArea`](mousearea.md) signals. Coordinates `x()` and `y()` are always translated into the receiving item's local cell space.

| Method | Return Type | Description |
| :--- | :--- | :--- |
| `type()` | `MouseEvent::Type` | `Type::Press`, `Type::Release`, `Type::Move`, or `Type::Wheel`. |
| `x()`, `y()` | `int` | Local cell coordinates relative to the receiving item. |
| `button()` | `Qt::MouseButton` | `Qt::LeftButton`, `Qt::MiddleButton`, `Qt::RightButton`, or `Qt::NoButton` (wheel and passive motion). |
| `nativeButton()` | `int` | Raw SGR low-bit button code (`0` left, `1` middle, `2` right, `3` none). |
| `modifiers()` | `int` | Active `Qt::KeyboardModifiers` bitmask. |
| `isMotion()` | `bool` | `true` when `type() == Type::Move`. |
| `wheelDeltaX()`, `wheelDeltaY()` | `int` | Horizontal and vertical wheel scroll deltas (`±1` per notch: `+1` wheel up, `-1` wheel down — unlike `QWheelEvent::angleDelta()`'s `±120` eighths-of-a-degree scale). |
| `isAccepted()` / `setAccepted(bool)` | `bool` / `void` | Reads or marks whether the event has been consumed. |

## `qmltui::Shortcut`

Application-wide shortcut evaluated before focused-item key dispatch (unless an editable input or running `PtyView` consumes an unmodified printable key). Exposed in QML as [`Shortcut`](../qml-types/shortcut.md).

- **`setSequence(const QString&)` / `setSequence(QKeyCombination)` / `setSequence(int)` / `setSequence(const QVariant&)`**: Accepts either portable single-chord strings (`"Ctrl+Q"`, `"Ctrl+Shift+P"`, `"Alt+Return"`, `"F5"`) or integer `Qt::KeyboardModifiers | Qt::Key` combinations.
- **`keyCombination() const`**: Returns the normalized `QKeyCombination::toCombined()` integer (`0` when unset or invalid).
- **`enabled()` / `setEnabled(bool)`** and **`activated()`** signal.

## `qmltui::InputScope`

Abstract interface implemented by overlay containers (such as [`Popup`](../qml-types/popup.md)) that participate in modal input trapping, `Escape` dismissal, and outside-click dismissal:

```cpp
class InputScope {
public:
    virtual ~InputScope() = default;
    virtual bool isScopeVisible() const = 0;
    virtual bool isScopeModal() const = 0;
    virtual bool closeOnOutsideClick() const = 0;
    virtual bool closeOnEscape() const = 0;
    virtual bool rejectScope() = 0;
    virtual Item* asItem() = 0;
    virtual const Item* asItem() const = 0;
    virtual bool isDescendantOfScope(const InputScope* other) const = 0;
};
```
