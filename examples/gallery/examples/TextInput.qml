pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property int cursorMode: 0
    property bool maskToken: true
    readonly property int currentCursorShape: cursorMode === 0 ? TextInput.Block : (cursorMode === 1 ? TextInput.Beam : TextInput.Underline)
    readonly property string cursorShapeName: cursorMode === 0 ? "Block" : (cursorMode === 1 ? "Beam" : "Underline")

    function cycleCursorShape() {
        root.cursorMode = (root.cursorMode + 1) % 3;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            title: Row {
                spacing: 1
                Text {
                    text: "TextInput · Single-Line Workbench"
                    color: "#f8fafc"
                }
                Text {
                    text: "[CORE]"
                    color: "#4ade80"
                }
            }
            titleTrailing: Text {
                text: "pos:" + nameInput.cursorPosition
                color: "#8b949e"
            }
            footer: Text {
                text: "Type text · Tab next control · Esc quit"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: root.cursorShapeName
                color: "#58a6ff"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 0

                Text {
                    text: "Primary Handle (max 24 chars):"
                    bold: true
                    color: "#4ade80"
                }

                Rectangle {
                    width: parent.width
                    height: 3
                    color: "#161b22"
                    border {
                        style: nameInput.activeFocus ? Border.Double : Border.Rounded
                        color: nameInput.activeFocus ? "#2cde85" : "#30363d"
                    }

                    TextInput {
                        id: nameInput
                        anchors {
                            fill: parent
                            margins: 1
                        }
                        focus: true
                        text: ""
                        placeholderText: "Type a username..."
                        placeholderColor: "#8b949e"
                        color: "#f8fafc"
                        maximumLength: 24
                        cursorShape: root.currentCursorShape
                    }
                }

                Text {
                    id: greetingText
                    width: parent.width
                    text: nameInput.text.length === 0 ? "Hello, stranger!" : "Hello, " + nameInput.text + "!"
                    bold: true
                    color: "#38bdf8"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "API Token (echoMode: Password):"
                    bold: true
                    color: "#fbbf24"
                }

                Rectangle {
                    width: parent.width
                    height: 3
                    color: "#161b22"
                    border {
                        style: tokenInput.activeFocus ? Border.Double : Border.Single
                        color: tokenInput.activeFocus ? "#fbbf24" : "#30363d"
                    }

                    TextInput {
                        id: tokenInput
                        anchors {
                            fill: parent
                            margins: 1
                        }
                        text: "sk-live-9042"
                        echoMode: root.maskToken ? TextInput.Password : TextInput.Normal
                        echoCharacter: "•"
                        placeholderText: "Enter secret token..."
                        placeholderColor: "#8b949e"
                        color: "#c9d1d9"
                        cursorShape: root.currentCursorShape
                    }
                }

                Text {
                    width: parent.width
                    text: "displayText: " + tokenInput.displayText
                    color: "#8b949e"
                }
            }
        }

        Panel {
            Layout.preferredWidth: 26
            Layout.fillHeight: true
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            title: Text {
                text: "Inspector"
                color: "#58a6ff"
            }
            footer: Text {
                text: "TextInput API"
                color: "#8b949e"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 0

                Text {
                    text: "PRIMARY TELEMETRY"
                    bold: true
                    color: "#4ade80"
                }

                Text {
                    text: "cursorPos: " + nameInput.cursorPosition
                    color: "#c9d1d9"
                }

                Text {
                    text: "length:    " + nameInput.text.length + "/" + nameInput.maximumLength
                    color: "#38bdf8"
                }

                Text {
                    text: "canUndo:   " + (nameInput.canUndo ? "true" : "false")
                    color: nameInput.canUndo ? "#2cde85" : "#8b949e"
                }

                Text {
                    text: "canRedo:   " + (nameInput.canRedo ? "true" : "false")
                    color: nameInput.canRedo ? "#2cde85" : "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "CURSOR & ECHO"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "cursorShape: " + root.cursorShapeName
                    color: "#c9d1d9"
                }

                Text {
                    text: "echoMode:    " + (root.maskToken ? "Password" : "Normal")
                    color: "#fbbf24"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "Cycle Cursor"
                    textColor: "#38bdf8"
                    onClicked: {
                        root.cycleCursorShape();
                        nameInput.focus = true;
                    }
                }

                CheckBox {
                    text: "Mask token"
                    checked: root.maskToken
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.maskToken = checked;
                    }
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "Clear Input"
                    textColor: "#f87171"
                    onClicked: {
                        nameInput.clear();
                        nameInput.focus = true;
                    }
                }
            }
        }
    }
}
