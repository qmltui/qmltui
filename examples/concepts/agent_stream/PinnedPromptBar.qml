import QtQuick
import Qtui
import Qtui.Controls

Column {
    id: promptBar

    Theme {
        id: theme
    }
    // 1-character/line vertical spacer above prompt bar
    Item {
        width: parent.width
        height: 1
    }
    required property bool ready
    required property bool working
    required property bool awaitingPermission
    required property bool modalActive
    required property string activeModel
    required property string sessionCostStr
    required property string currentToolName
    required property string currentToolTarget
    required property string currentToolAction
    required property int permissionIndex
    required property var spinnerFrames
    required property int spinnerIndex
    required property string workingLabel

    property alias inputText: inputField.text
    property alias inputCursorPosition: inputField.cursorPosition
    property alias popupVisible: cmdPopup.visible
    property bool userNavigated: false

    signal requestSubmitted(string text)
    signal permissionChosen(int index)
    signal toggleShellModeRequested
    signal openModelPickerRequested
    signal openDiffInspectorRequested

    function focusPrompt() {
        inputField.focus = true;
    }

    width: parent ? parent.width : 80
    spacing: 0

    // Top Status Header (Ready & Working)
    Row {
        visible: !promptBar.modalActive && !promptBar.awaitingPermission
        width: parent.width
        height: 1
        spacing: 1

        StatusBadge {
            text: promptBar.working ? "RUNNING" : "ONLINE"
            status: promptBar.working ? "warning" : "success"
            variant: "subtle"
            showIcon: false
        }

        Text {
            text: promptBar.activeModel
            color: theme.cyan
            bold: true
        }
        Text {
            text: "· ~/Qtui ·"
            color: theme.muted
        }
        Text {
            text: promptBar.sessionCostStr
            color: theme.green
            bold: true
        }
        Text {
            text: " · [F2] Model · [F3] Diff · [Ctrl+T] Shell"
            color: theme.amber
        }
    }

    // Working State Line
    Row {
        visible: !promptBar.modalActive && promptBar.working
        width: parent.width
        height: 1
        spacing: 0

        Text {
            text: promptBar.spinnerFrames[promptBar.spinnerIndex] + " "
            color: theme.cyan
            bold: true
        }
        Text {
            text: promptBar.workingLabel
            color: "white"
        }
        Text {
            text: " · Ctrl+C to abort"
            color: theme.muted
        }
    }

    // Ready Prompt Area (with Autocomplete Popup)
    Column {
        visible: !promptBar.modalActive && promptBar.ready
        width: parent.width
        spacing: 0

        CommandPalette {
            id: cmdPopup
            width: parent.width
            filterText: inputField.text
            onCommandSelected: cmd => {
                inputField.text = cmd + " ";
                inputField.cursorPosition = inputField.text.length;
                promptBar.userNavigated = false;
            }
            onCommandExecuted: cmd => {
                promptBar.userNavigated = false;
                promptBar.requestSubmitted(cmd);
                inputField.text = "";
            }
        }

        Row {
            width: parent.width
            height: 1
            spacing: 0

            Text {
                text: "❯ "
                color: theme.cyan
                bold: true
            }

            TextInput {
                id: inputField
                width: parent.width - 4
                color: "white"
                placeholderText: "Ask anything, type / for commands, F2: model, F3: diff, Ctrl+T: shell..."
                placeholderColor: theme.muted
                cursorBlinking: false
                cursorShape: TextInput.Block
                focus: promptBar.ready && !promptBar.modalActive
                onTextChanged: {
                    promptBar.userNavigated = false;
                    if (inputField.text.startsWith("/")) {
                        const trimmed = inputField.text.trim();
                        if (trimmed.indexOf(" ") !== -1 && inputField.text.endsWith(" ")) {
                            cmdPopup.close();
                        } else {
                            cmdPopup.open();
                        }
                    } else {
                        cmdPopup.close();
                    }
                }
                onAccepted: {
                    cmdPopup.close();
                    promptBar.userNavigated = false;
                    const trimmed = inputField.text.trim();
                    if (trimmed.length > 0) {
                        promptBar.requestSubmitted(trimmed);
                        inputField.text = "";
                    }
                }

                Keys.priority: Keys.BeforeItem
                Keys.onPressed: event => {
                    if (cmdPopup.visible) {
                        if (event.key === Qt.Key_Up) {
                            promptBar.userNavigated = true;
                            cmdPopup.selectPrev();
                            event.accepted = true;
                            return;
                        } else if (event.key === Qt.Key_Down) {
                            promptBar.userNavigated = true;
                            cmdPopup.selectNext();
                            event.accepted = true;
                            return;
                        } else if (event.key === Qt.Key_Tab || event.key === Qt.Key_Right) {
                            cmdPopup.chooseCurrent();
                            event.accepted = true;
                            return;
                        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                            if (promptBar.userNavigated && cmdPopup.currentCommand.length > 0) {
                                cmdPopup.executeCurrent();
                            } else {
                                const trimmed = inputField.text.trim();
                                cmdPopup.close();
                                promptBar.userNavigated = false;
                                if (trimmed.length > 0) {
                                    promptBar.requestSubmitted(trimmed);
                                    inputField.text = "";
                                }
                            }
                            event.accepted = true;
                            return;
                        } else if (event.key === Qt.Key_Escape) {
                            cmdPopup.close();
                            promptBar.userNavigated = false;
                            event.accepted = true;
                            return;
                        }
                    }

                    if ((event.modifiers & Qt.ControlModifier) && (event.key === Qt.Key_T || event.key === Qt.Key_B)) {
                        promptBar.toggleShellModeRequested();
                        event.accepted = true;
                    } else if (event.key === Qt.Key_F1) {
                        promptBar.toggleShellModeRequested();
                        event.accepted = true;
                    } else if (event.key === Qt.Key_F2) {
                        promptBar.openModelPickerRequested();
                        event.accepted = true;
                    } else if (event.key === Qt.Key_F3) {
                        promptBar.openDiffInspectorRequested();
                        event.accepted = true;
                    }
                }
            }
        }
    }

    // Security Permission Dialog
    Column {
        visible: !promptBar.modalActive && promptBar.awaitingPermission
        width: parent.width
        spacing: 0

        Row {
            width: parent.width
            height: 1
            spacing: 1

            StatusBadge {
                text: "GATEWAY"
                status: "warning"
                variant: "subtle"
                showIcon: false
            }

            Text {
                text: promptBar.currentToolName + "(" + promptBar.currentToolTarget + ")"
                color: theme.cyan
                bold: true
            }
        }

        Row {
            width: parent.width
            height: 1
            spacing: 0
            Text {
                text: "  Action: "
                color: theme.muted
            }
            Text {
                text: promptBar.currentToolAction
                color: "white"
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: promptBar.permissionIndex === 0 ? theme.amber : "transparent"
            Text {
                text: (promptBar.permissionIndex === 0 ? "  > [1] " : "    [1] ") + "Allow once"
                color: promptBar.permissionIndex === 0 ? "black" : "white"
                bold: promptBar.permissionIndex === 0
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: promptBar.permissionIndex === 1 ? theme.amber : "transparent"
            Text {
                text: (promptBar.permissionIndex === 1 ? "  > [2] " : "    [2] ") + "Always allow for this session"
                color: promptBar.permissionIndex === 1 ? "black" : "white"
                bold: promptBar.permissionIndex === 1
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: promptBar.permissionIndex === 2 ? theme.amber : "transparent"
            Text {
                text: (promptBar.permissionIndex === 2 ? "  > [3] " : "    [3] ") + "Deny execution"
                color: promptBar.permissionIndex === 2 ? "black" : "white"
                bold: promptBar.permissionIndex === 2
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: promptBar.permissionIndex === 3 ? theme.amber : "transparent"
            Text {
                text: (promptBar.permissionIndex === 3 ? "  > [4] " : "    [4] ") + "Cancel request"
                color: promptBar.permissionIndex === 3 ? "black" : "white"
                bold: promptBar.permissionIndex === 3
            }
        }
    }
}
