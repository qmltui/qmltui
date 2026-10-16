# 03: Input and focus

The input starts focused. Tab moves to the button, and Enter activates its
`onClicked` handler. Greeting text is a binding to application state. Shortcut
provides a F5 command independent of focus. Escape bubbles to the root only
when a child does not consume it.

Build with `cmake --build build --target qmltui_tutorial_03_input_and_focus` and run
`./build/examples/tutorials/03_input_and_focus/qmltui_tutorial_03_input_and_focus`. Press Escape to quit.

## Terminal Output Preview

```text

  Enter your name

  Name

  ┌───────┐
  │ Greet │
  └───────┘

  Tab moves focus; Enter activates; F5 resets

```

## Source Files

### `Main.qml`

<!-- include: examples/tutorials/03_input_and_focus/Main.qml -->
```qml
pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
    property string greeting: "Enter your name"
    Shortcut { sequence: Qt.Key_F5; onActivated: root.greeting = "Enter your name" }
    Column {
        x: 2; y: 1; spacing: 1
        Text { text: root.greeting }
        TextInput { id: nameInput; width: 24; focus: true; placeholderText: "Name" }
        Button { text: "Greet"; onClicked: root.greeting = "Hello, " + nameInput.text + "!" }
        Text { text: "Tab moves focus; Enter activates; F5 resets" }
    }
}
```

## Key Concepts in This Chapter

- **Initial Focus (`focus: true`) & Tab Traversal**: Setting `focus: true` on `nameInput` gives the `TextInput` active focus when the scene loads. Pressing **Tab** (or **Shift+Tab**) cycles focus through focusable controls (`TextInput` → `Button`) in tree order. When `Button` receives active focus, its border switches from single (`┌───┐`) to double (`╔═══╗`).
- **Reactive Property Bindings**: `root.greeting` is declared as a custom `property string` on the root `Terminal`. The top `Text { text: root.greeting }` binds to it declaratively, so updating `root.greeting` inside `Button.onClicked` immediately schedules a repaint of the affected cells.
- **`Shortcut` vs. `Keys.onPressed` Bubbling**: `Shortcut { sequence: Qt.Key_F5; ... }` triggers application-wide regardless of which control currently holds focus. By contrast, `Keys.onPressed` events start at the actively focused item and bubble up the parent chain until an item sets `event.accepted = true`—allowing modals or inputs to handle keys first before `Escape` reaches `root`.

---

[← Previous: 02: Layout and text](02-layout-and-text.md) | [Tutorial index](index.md) | [Next: 04: Models and views →](04-models-and-views.md)
