import QtQml
import QmlTui
import QmlTui.Controls

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
    signal openModelPickerRequested
    signal openDiffInspectorRequested
    signal interruptRequested

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
            text: promptBar.working ? "running" : "online"
            status: promptBar.working ? "warning" : "success"
            variant: "subtle"
            showIcon: false
        }

        Rectangle {
            width: modelChipText.implicitWidth
            height: 1
            color: modelChipMouse.containsMouse ? "#1e293b" : "transparent"

            Text {
                id: modelChipText
                anchors.fill: parent
                text: promptBar.activeModel
                color: modelChipMouse.containsMouse ? theme.white : theme.cyan
                bold: true
                underline: modelChipMouse.containsMouse
            }

            MouseArea {
                id: modelChipMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: promptBar.openModelPickerRequested()
            }
        }

        Text {
            text: "· ~/QmlTui ·"
            color: theme.muted
        }

        Text {
            text: promptBar.sessionCostStr
            color: theme.green
            bold: true
        }

        Row {
            spacing: 0
            height: 1

            Text {
                text: " · "
                color: theme.muted
            }

            Rectangle {
                width: modelBtnText.implicitWidth
                height: 1
                color: modelBtnMouse.containsMouse ? "#1e293b" : "transparent"

                Text {
                    id: modelBtnText
                    anchors.fill: parent
                    text: "[F2] model"
                    color: modelBtnMouse.containsMouse ? theme.white : theme.amber
                    bold: modelBtnMouse.containsMouse
                    underline: modelBtnMouse.containsMouse
                }

                MouseArea {
                    id: modelBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: promptBar.openModelPickerRequested()
                }
            }

            Text {
                text: " · "
                color: theme.muted
            }

            Rectangle {
                width: diffBtnText.implicitWidth
                height: 1
                color: diffBtnMouse.containsMouse ? "#1e293b" : "transparent"

                Text {
                    id: diffBtnText
                    anchors.fill: parent
                    text: "[F3] diff"
                    color: diffBtnMouse.containsMouse ? theme.white : theme.amber
                    bold: diffBtnMouse.containsMouse
                    underline: diffBtnMouse.containsMouse
                }

                MouseArea {
                    id: diffBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: promptBar.openDiffInspectorRequested()
                }
            }
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
            text: " · "
            color: theme.muted
        }
        Rectangle {
            width: abortBtnText.implicitWidth
            height: 1
            color: abortBtnMouse.containsMouse ? "#3b1219" : "transparent"

            Text {
                id: abortBtnText
                anchors.fill: parent
                text: "Ctrl+C to abort"
                color: abortBtnMouse.containsMouse ? theme.rose : theme.muted
                bold: abortBtnMouse.containsMouse
                underline: abortBtnMouse.containsMouse
            }

            MouseArea {
                id: abortBtnMouse
                anchors.fill: parent
                hoverEnabled: true
                onClicked: promptBar.interruptRequested()
            }
        }
    }

    // Ready Prompt Area (with Autocomplete Popup)
    Column {
        id: promptArea
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

            Rectangle {
                width: 2
                height: 1
                color: chevronMouse.containsMouse ? "#1e293b" : "transparent"

                Text {
                    anchors.fill: parent
                    text: "❯ "
                    color: chevronMouse.containsMouse ? theme.white : theme.cyan
                    bold: true
                }

                MouseArea {
                    id: chevronMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: promptBar.focusPrompt()
                }
            }

            TextInput {
                id: inputField
                width: parent.width - 4
                color: "white"
                placeholderText: "Ask anything, type / for commands, F2: model, F3: diff..."
                placeholderColor: theme.muted
                cursorBlinking: false
                cursorShape: TextInput.Block
                // Request focus only after the containing prompt becomes visible.
                focus: promptArea.visible
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

                    if (event.key === Qt.Key_F2) {
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
                text: "Permission"
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
            color: (promptBar.permissionIndex === 0 || perm0Mouse.containsMouse) ? (perm0Mouse.pressed ? "#d97706" : theme.amber) : "transparent"
            Text {
                text: ((promptBar.permissionIndex === 0 || perm0Mouse.containsMouse) ? "  > [1] " : "    [1] ") + "Allow once"
                color: (promptBar.permissionIndex === 0 || perm0Mouse.containsMouse) ? "black" : "white"
                bold: promptBar.permissionIndex === 0 || perm0Mouse.containsMouse
                underline: perm0Mouse.containsMouse
            }
            MouseArea {
                id: perm0Mouse
                anchors.fill: parent
                hoverEnabled: true
                onEntered: promptBar.permissionIndex = 0
                onClicked: promptBar.permissionChosen(0)
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: (promptBar.permissionIndex === 1 || perm1Mouse.containsMouse) ? (perm1Mouse.pressed ? "#d97706" : theme.amber) : "transparent"
            Text {
                text: ((promptBar.permissionIndex === 1 || perm1Mouse.containsMouse) ? "  > [2] " : "    [2] ") + "Always allow for this session"
                color: (promptBar.permissionIndex === 1 || perm1Mouse.containsMouse) ? "black" : "white"
                bold: promptBar.permissionIndex === 1 || perm1Mouse.containsMouse
                underline: perm1Mouse.containsMouse
            }
            MouseArea {
                id: perm1Mouse
                anchors.fill: parent
                hoverEnabled: true
                onEntered: promptBar.permissionIndex = 1
                onClicked: promptBar.permissionChosen(1)
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: (promptBar.permissionIndex === 2 || perm2Mouse.containsMouse) ? (perm2Mouse.pressed ? "#d97706" : theme.amber) : "transparent"
            Text {
                text: ((promptBar.permissionIndex === 2 || perm2Mouse.containsMouse) ? "  > [3] " : "    [3] ") + "Deny execution"
                color: (promptBar.permissionIndex === 2 || perm2Mouse.containsMouse) ? "black" : "white"
                bold: promptBar.permissionIndex === 2 || perm2Mouse.containsMouse
                underline: perm2Mouse.containsMouse
            }
            MouseArea {
                id: perm2Mouse
                anchors.fill: parent
                hoverEnabled: true
                onEntered: promptBar.permissionIndex = 2
                onClicked: promptBar.permissionChosen(2)
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: (promptBar.permissionIndex === 3 || perm3Mouse.containsMouse) ? (perm3Mouse.pressed ? "#d97706" : theme.amber) : "transparent"
            Text {
                text: ((promptBar.permissionIndex === 3 || perm3Mouse.containsMouse) ? "  > [4] " : "    [4] ") + "Cancel request"
                color: (promptBar.permissionIndex === 3 || perm3Mouse.containsMouse) ? "black" : "white"
                bold: promptBar.permissionIndex === 3 || perm3Mouse.containsMouse
                underline: perm3Mouse.containsMouse
            }
            MouseArea {
                id: perm3Mouse
                anchors.fill: parent
                hoverEnabled: true
                onEntered: promptBar.permissionIndex = 3
                onClicked: promptBar.permissionChosen(3)
            }
        }
    }
}
