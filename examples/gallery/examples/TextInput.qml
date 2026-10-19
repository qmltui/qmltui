pragma ComponentBehavior: Bound
import QmlTui

// TextInput: Single-line text editor with placeholder, password masking, and regex validation.
Column {
    spacing: 1

    Text {
        text: "Type to edit; Enter emits accepted(). Tab moves to the next field."
        color: "#8b949e"
    }

    // Standard single-line text input
    Row {
        spacing: 1

        Text {
            width: 10
            text: "Name"
            color: "#c9d1d9"
        }

        Rectangle {
            width: 28
            height: 1
            color: "#161b22"

            TextInput {
                id: nameInput
                anchors.fill: parent
                focus: true
                placeholderText: "Your name"
                color: "#4ade80"
            }
        }
    }

    // Password echo mode masks typed characters
    Row {
        spacing: 1

        Text {
            width: 10
            text: "Password"
            color: "#c9d1d9"
        }

        Rectangle {
            width: 28
            height: 1
            color: "#161b22"

            TextInput {
                id: passwordInput
                anchors.fill: parent
                echoMode: TextInput.Password
                placeholderText: "secret"
                color: "#4ade80"
            }
        }
    }

    // Regular expression validator and maximumLength
    Row {
        spacing: 1

        Text {
            width: 10
            text: "Digits"
            color: "#c9d1d9"
        }

        Rectangle {
            width: 28
            height: 1
            color: "#161b22"

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
