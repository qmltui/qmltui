# 05: Controls and styling

Checkbox and switch handle toggles. The slider value feeds both a progress
bar and a label through bindings. An inline AppButton component owns shared
styling without a framework palette. The modal consumes Escape before the
application's root quit handler; accepting or rejecting updates visible state.

Build with `cmake --build build --target qmltui_tutorial_05_controls_and_styling` and run
`./build/examples/tutorials/05_controls_and_styling/qmltui_tutorial_05_controls_and_styling`. Press Escape to quit.

## Terminal Output Preview

```text

  Settings ready

  [ ] Logging

  Logging disabled

  [OFF] Notifications

  ──────────●────────────── [40]

  ████████████░░░░░░░░░░░░░░░░░░

  Volume: 40%

  ┌────────────────┐
  │ Apply settings │
  └────────────────┘

```

## Source Files

### `Main.qml`

<!-- include: examples/tutorials/05_controls_and_styling/Main.qml -->
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
    property string status: "Settings ready"
    component AppButton: Button { textColor: "cyan" }
    Column {
        x: 2; y: 1; spacing: 1
        Text { text: root.status }
        CheckBox { id: logging; text: "Logging"; focus: true }
        Text { text: logging.checked ? "Logging enabled" : "Logging disabled" }
        Switch { text: "Notifications" }
        Slider { id: volume; width: 30; from: 0; to: 100; value: 40 }
        ProgressBar { width: 30; from: 0; to: 100; value: volume.value }
        Text { text: "Volume: " + volume.value + "%" }
        AppButton { text: "Apply settings"; onClicked: confirmation.open() }
    }
    Dialog {
        id: confirmation
        title: "Apply settings?"
        message: "Save these settings now?"
        onAccepted: root.status = "Settings saved"
        onRejected: root.status = "Settings unchanged"
    }
}
```

### `test_tutorial.js`

<!-- include: examples/tutorials/05_controls_and_styling/test_tutorial.js -->
```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Logging disabled");
    terminal.press("Space");
    terminal.expectText("Logging enabled");
    terminal.waitForText("40%");
    terminal.clickText("Apply settings");
    terminal.expectText("Save these settings now?");
    terminal.press("Escape");
    terminal.expectText("Settings unchanged");
    terminal.captureText("tutorial.txt");
}
```

## Key Concepts in This Chapter

- **Application-Owned Styling (`component AppButton: Button`)**: QmlTui controls default to `"default"` terminal colors and do not enforce a global framework theme singleton. To share styling across an application, define reusable QML components—either as inline components (`component AppButton: Button { textColor: "cyan" }`) or separate `.qml` files. Explicit color overrides survive focus and disabled state changes.
- **Synchronized Controls via Bindings**: `Slider` (`id: volume`) exposes its numeric `value`, while `ProgressBar { value: volume.value }` and `Text { text: "Volume: " + volume.value + "%" }` bind to it directly. Moving the slider with **Left**/**Right** arrows or the mouse updates both dependents in the same frame.
- **Modal `Dialog` & Contextual `Escape`**: Calling `confirmation.open()` displays a modal `Dialog` overlay that traps keyboard focus. Pressing **Escape** while the dialog is open dismisses the dialog and emits `onRejected` (`root.status = "Settings unchanged"`), consuming the key event so it does **not** trigger the root `Terminal`'s quit handler.

---

[← Previous: 04: Models and views](04-models-and-views.md) | [Tutorial index](index.md) | [Next: 06: Testing with runner →](06-testing-with-runner.md)
