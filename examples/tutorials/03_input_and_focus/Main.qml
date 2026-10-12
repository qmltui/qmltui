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
