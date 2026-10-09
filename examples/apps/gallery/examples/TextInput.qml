pragma ComponentBehavior: Bound
import Qtui

// TextInput: Single-line editing. One tile of the Qtui gallery (examples/apps/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Column {
        x: 2
        y: 1
        spacing: 1
        Text { text: "Type to edit; Enter emits accepted(). Tab moves to the next field."; color: "#6e7b8b" }
        Row {
            spacing: 1
            Text { width: 10; text: "Name"; color: "#c9d1d9" }
            Rectangle {
                width: 30; height: 1; color: "#141b24"
                TextInput {
                    id: nameInput
                    anchors.fill: parent
                    focus: true
                    placeholderText: "Your name"
                    color: "#4ade80"
                }
            }
        }
        Row {
            spacing: 1
            Text { width: 10; text: "Password"; color: "#c9d1d9" }
            Rectangle {
                width: 30; height: 1; color: "#141b24"
                TextInput {
                    id: passwordInput
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    placeholderText: "secret"
                    color: "#4ade80"
                }
            }
        }
        Row {
            spacing: 1
            Text { width: 10; text: "Digits"; color: "#c9d1d9" }
            Rectangle {
                width: 30; height: 1; color: "#141b24"
                TextInput {
                    id: digitsInput
                    anchors.fill: parent
                    placeholderText: "0-9 only"
                    validator: /^[0-9]*$/
                    maximumLength: 8
                    color: digitsInput.acceptableInput ? "#4ade80" : "#f87171"
                }
            }
        }
        Text {
            text: "Hello, " + (nameInput.text.length ? nameInput.text : "stranger") + "! Password has " + passwordInput.text.length + " characters."
            color: "#4ade80"
        }
    }
}
