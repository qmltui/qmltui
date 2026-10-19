pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property int clicks: 0
    property string lastAction: "None"
    property bool buttonsEnabled: true
    property bool forceCheckable: false
    readonly property bool anyDown: primaryBtn.down || secondaryBtn.down || filledBtn.down || pinBtn.down || holdBtn.down || saveBtn.down || dangerBtn.down
    readonly property string focusedButtonName: {
        if (primaryBtn.activeFocus)
            return "Primary";
        if (secondaryBtn.activeFocus)
            return "Secondary";
        if (filledBtn.activeFocus)
            return "Filled";
        if (pinBtn.activeFocus)
            return "Pin";
        if (holdBtn.activeFocus)
            return "Hold me";
        if (saveBtn.activeFocus)
            return "Save";
        if (dangerBtn.activeFocus)
            return "Reset";
        if (enabledToggle.activeFocus)
            return "enabled";
        if (checkableToggle.activeFocus)
            return "checkable";
        if (pinToggle.activeFocus)
            return "pin.checked";
        return "None";
    }
    readonly property string hoveredButtonName: {
        if (primaryBtn.hovered)
            return "Primary";
        if (secondaryBtn.hovered)
            return "Secondary";
        if (filledBtn.hovered)
            return "Filled";
        if (pinBtn.hovered)
            return "Pin";
        if (holdBtn.hovered)
            return "Hold me";
        if (saveBtn.hovered)
            return "Save";
        if (dangerBtn.hovered)
            return "Reset";
        return "None";
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
                    text: "Button"
                    color: "#2cde85"
                }
                Text {
                    text: "Controls"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.buttonsEnabled ? "ACTIVE" : "DISABLED"
                color: root.buttonsEnabled ? "#2cde85" : "#f87171"
            }
            footer: Text {
                text: "Enter/Space activate"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "Tab next"
                color: "#8b949e"
            }
            Column {
                anchors {
                    fill: parent
                    leftMargin: 2
                    rightMargin: 2
                    topMargin: 1
                    bottomMargin: 1
                }
                spacing: 1

                Text {
                    text: root.clicks === 0 ? "No clicks yet" : (root.clicks === 1 ? "Clicked 1 time" : "Clicked " + root.clicks + " times")
                    color: root.clicks > 0 ? "#2cde85" : "#8b949e"
                    bold: true
                }

                Text {
                    text: "Last action: " + root.lastAction + "  |  Pin: " + (pinBtn.checked ? "ON" : "OFF")
                    color: "#c9d1d9"
                }

                Row {
                    spacing: 1

                    Button {
                        id: primaryBtn

                        focus: true
                        hoverEnabled: true
                        enabled: root.buttonsEnabled
                        checkable: root.forceCheckable
                        text: "Primary"
                        textColor: "#2cde85"

                        onClicked: {
                            root.clicks += 1;
                            root.lastAction = "Primary";
                        }
                    }

                    Button {
                        id: secondaryBtn

                        hoverEnabled: true
                        enabled: root.buttonsEnabled
                        checkable: root.forceCheckable
                        text: "Secondary"
                        textColor: "#c9d1d9"

                        onClicked: {
                            root.clicks += 1;
                            root.lastAction = "Secondary";
                        }
                    }

                    Button {
                        id: filledBtn

                        hoverEnabled: true
                        enabled: root.buttonsEnabled
                        checkable: root.forceCheckable
                        text: "Filled"
                        textColor: "#0d1117"
                        background: Rectangle {
                            color: "#2cde85"
                            border {
                                style: Border.None
                            }
                        }

                        onClicked: {
                            root.clicks += 1;
                            root.lastAction = "Filled";
                        }
                    }
                }

                Row {
                    spacing: 1

                    Button {
                        id: pinBtn

                        checkable: true
                        hoverEnabled: true
                        enabled: root.buttonsEnabled
                        text: checked ? "★ Pinned" : "☆ Pin"
                        textColor: checked ? "#f9c74f" : "#c9d1d9"

                        onClicked: {
                            root.clicks += 1;
                            root.lastAction = checked ? "Pinned" : "Unpinned";
                        }
                    }

                    Button {
                        id: holdBtn

                        hoverEnabled: true
                        enabled: root.buttonsEnabled
                        text: down ? "Holding!" : "Hold me"
                        textColor: down ? "#7ee0ff" : "#58a6ff"

                        onClicked: {
                            root.clicks += 1;
                            root.lastAction = "Hold me";
                        }
                    }

                    Button {
                        id: saveBtn

                        hoverEnabled: true
                        enabled: root.buttonsEnabled
                        checkable: root.forceCheckable
                        text: "✓ Save"
                        textColor: "#38bdf8"

                        onClicked: {
                            root.clicks += 1;
                            root.lastAction = "Save";
                        }
                    }
                }

                Row {
                    spacing: 2

                    Button {
                        id: dangerBtn

                        hoverEnabled: true
                        enabled: root.buttonsEnabled
                        text: "Reset"
                        textColor: "#f87171"

                        onClicked: {
                            root.clicks = 0;
                            pinBtn.checked = false;
                            root.lastAction = "Reset";
                        }
                    }

                    Column {
                        spacing: 0

                        Text {
                            text: "Down state: " + (root.anyDown ? "PRESSED" : "IDLE")
                            color: root.anyDown ? "#7ee0ff" : "#8b949e"
                        }

                        Text {
                            text: "Mode: " + (root.forceCheckable ? "Checkable" : "Push")
                            color: "#8b949e"
                        }
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 28
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
            Column {
                anchors {
                    fill: parent
                    leftMargin: 2
                    rightMargin: 2
                    topMargin: 1
                    bottomMargin: 1
                }
                spacing: 1

                Column {
                    spacing: 0

                    Text {
                        text: "PROPERTIES"
                        color: "#58a6ff"
                        bold: true
                    }

                    CheckBox {
                        id: enabledToggle

                        checked: root.buttonsEnabled
                        text: "enabled"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.buttonsEnabled = checked;
                        }
                    }

                    CheckBox {
                        id: checkableToggle

                        checked: root.forceCheckable
                        text: "checkable"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.forceCheckable = checked;
                        }
                    }

                    CheckBox {
                        id: pinToggle

                        checked: pinBtn.checked
                        text: "pin.checked"
                        textColor: "#f9c74f"

                        onToggled: checked => {
                            pinBtn.checked = checked;
                            root.lastAction = checked ? "Pinned" : "Unpinned";
                        }
                    }
                }

                Column {
                    spacing: 0

                    Text {
                        text: "TELEMETRY"
                        color: "#58a6ff"
                        bold: true
                    }

                    Text {
                        text: "clicks: " + root.clicks
                        color: "#2cde85"
                    }

                    Text {
                        text: "pin.checked: " + (pinBtn.checked ? "true" : "false")
                        color: pinBtn.checked ? "#f9c74f" : "#8b949e"
                    }

                    Text {
                        text: "focus: " + root.focusedButtonName
                        color: "#c9d1d9"
                    }

                    Text {
                        text: "hover: " + root.hoveredButtonName
                        color: "#7ee0ff"
                    }

                    Text {
                        text: "last: " + root.lastAction
                        color: "#c9d1d9"
                    }
                }
            }
        }
    }
}
