pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// FocusScope: Each scope remembers its own focused child when focus moves
// between scopes. Press 1 / 2 (or click a pane) to switch the active scope.
Column {
    id: scopeDemo

    property string activePane: leftScope.activeFocus ? "Servers" : (rightScope.activeFocus ? "Actions" : "None")
    property string lastAction: "each scope keeps its own focused child"

    anchors.centerIn: parent
    spacing: 1

    Shortcut {
        sequence: Qt.Key_1
        onActivated: leftScope.focus = true
    }

    Shortcut {
        sequence: Qt.Key_2
        onActivated: rightScope.focus = true
    }

    Text {
        text: "Press 1 (Servers) or 2 (Actions) to switch active scope:"
        color: "#8b949e"
    }

    Row {
        spacing: 2

        FocusScope {
            id: leftScope
            width: 28
            height: 7
            focus: true

            Rectangle {
                anchors.fill: parent
                color: leftScope.activeFocus ? "#111927" : "#161b22"

                Text {
                    x: 2
                    y: 1
                    text: "[1] Servers"
                    color: leftScope.activeFocus ? "#58a6ff" : "#8b949e"
                    bold: leftScope.activeFocus
                }

                Text {
                    anchors {
                        right: parent.right
                        rightMargin: 2
                    }
                    y: 1
                    text: leftScope.activeFocus ? "● active" : "idle"
                    color: leftScope.activeFocus ? "#2cde85" : "#8b949e"
                }

                Column {
                    x: 2
                    y: 3
                    spacing: 1

                    Button {
                        id: srvBtn1
                        width: 24
                        height: 1
                        text: "api-gateway-01"
                        focus: true
                        textColor: srvBtn1.activeFocus ? "#0d1117" : "#58a6ff"
                        background: Rectangle {
                            color: srvBtn1.activeFocus ? "#58a6ff" : "#21262d"
                        }
                        onClicked: scopeDemo.lastAction = "Selected api-gateway-01"
                    }

                    Button {
                        id: srvBtn2
                        width: 24
                        height: 1
                        text: "worker-pool-02"
                        textColor: srvBtn2.activeFocus ? "#0d1117" : "#58a6ff"
                        background: Rectangle {
                            color: srvBtn2.activeFocus ? "#58a6ff" : "#21262d"
                        }
                        onClicked: scopeDemo.lastAction = "Selected worker-pool-02"
                    }
                }
            }
        }

        FocusScope {
            id: rightScope
            width: 28
            height: 7

            Rectangle {
                anchors.fill: parent
                color: rightScope.activeFocus ? "#122d24" : "#161b22"

                Text {
                    x: 2
                    y: 1
                    text: "[2] Actions"
                    color: rightScope.activeFocus ? "#2cde85" : "#8b949e"
                    bold: rightScope.activeFocus
                }

                Text {
                    anchors {
                        right: parent.right
                        rightMargin: 2
                    }
                    y: 1
                    text: rightScope.activeFocus ? "● active" : "idle"
                    color: rightScope.activeFocus ? "#2cde85" : "#8b949e"
                }

                Column {
                    x: 2
                    y: 3
                    spacing: 1

                    Button {
                        id: actBtn1
                        width: 24
                        height: 1
                        text: "Restart Service"
                        focus: true
                        textColor: actBtn1.activeFocus ? "#0d1117" : "#2cde85"
                        background: Rectangle {
                            color: actBtn1.activeFocus ? "#2cde85" : "#21262d"
                        }
                        onClicked: scopeDemo.lastAction = "Triggered: Restart Service"
                    }

                    Button {
                        id: actBtn2
                        width: 24
                        height: 1
                        text: "Drain Connections"
                        textColor: actBtn2.activeFocus ? "#0d1117" : "#f59e0b"
                        background: Rectangle {
                            color: actBtn2.activeFocus ? "#f59e0b" : "#21262d"
                        }
                        onClicked: scopeDemo.lastAction = "Triggered: Drain Connections"
                    }
                }
            }
        }
    }

    Text {
        text: "Active scope: " + scopeDemo.activePane + "  ·  " + scopeDemo.lastAction
        color: "#58a6ff"
    }
}
