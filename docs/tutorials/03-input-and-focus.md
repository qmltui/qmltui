# 03: Input and focus

The input starts focused. Tab moves to the button, and Enter activates its
`onClicked` handler. Greeting text is a binding to application state. Shortcut
provides a F5 command independent of focus. Escape bubbles to the root only
when a child does not consume it.

Build with `cmake --build build --target qmltui_tutorial_03_input_and_focus` and run
`./build/examples/tutorials/03_input_and_focus/qmltui_tutorial_03_input_and_focus`. Press Escape to quit.

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

<!-- include: examples/tutorials/03_input_and_focus/test_tutorial.js -->
```javascript
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.typeText("Ada");
    terminal.press("Tab");
    terminal.press("Enter");
    terminal.expectText("Hello, Ada!");
    terminal.captureText("tutorial.txt");
}
```

[Tutorial index](index.md)
