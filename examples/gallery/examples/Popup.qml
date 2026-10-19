pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property bool dimBg: true
    property int openCount: 0
    property int closeCount: 0
    property string lastAction: "None"

    function triggerAction(label) {
        lastAction = label;
        commandPopup.close();
        openPopupBtn.focus = true;
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
                    text: "Popup Overlay Stage"
                    color: "#a3e635"
                }
                Text {
                    text: "[Controls]"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: commandPopup.visible ? "ACTIVE" : "READY"
                color: commandPopup.visible ? "#2cde85" : "#8b949e"
            }
            footer: Text {
                text: "Enter open · Esc dismiss overlay"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "Popup"
                color: "#8b949e"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Cluster Command Launcher"
                    color: "#f0f6fc"
                    bold: true
                }

                Text {
                    text: "Last Command: " + root.lastAction
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "Dim Background: " + (root.dimBg ? "enabled" : "disabled")
                    color: "#8b949e"
                }

                Button {
                    id: openPopupBtn

                    focus: true
                    text: "Open Command Popup"
                    textColor: "#a3e635"
                    onClicked: commandPopup.open()
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
                x: 2
                y: 2
                spacing: 1

                CheckBox {
                    id: dimCheck

                    text: "dimBackground"
                    checked: root.dimBg
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.dimBg = checked;
                    }
                }

                Text {
                    text: "Opened: " + root.openCount
                    color: "#58a6ff"
                    bold: true
                }

                Text {
                    text: "Closed: " + root.closeCount
                    color: "#f0f6fc"
                }

                Text {
                    text: "Last Action:"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.lastAction
                    color: "#2cde85"
                    bold: true
                }
            }
        }
    }

    Popup {
        id: commandPopup

        width: 34
        height: 10
        anchors.centerIn: parent
        modal: true
        dimBackground: root.dimBg
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnOutsideClick
        initialFocusItem: syncActionBtn
        onOpened: root.openCount += 1
        onClosed: {
            root.closeCount += 1;
            openPopupBtn.focus = true;
        }

        Panel {
            anchors.fill: parent
            color: "#161b22"
            border {
                style: Border.Double
                color: "#a3e635"
            }

            title: Text {
                text: "Command Palette"
                color: "#a3e635"
            }
            footer: Text {
                text: "Esc to close"
                color: "#8b949e"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Select a quick action:"
                    color: "#c9d1d9"
                }

                Row {
                    spacing: 1

                    Button {
                        id: syncActionBtn

                        text: "Sync"
                        textColor: "#2cde85"
                        onClicked: root.triggerAction("Sync Nodes")
                    }

                    Button {
                        id: drainActionBtn

                        text: "Drain"
                        textColor: "#fbbf24"
                        onClicked: root.triggerAction("Drain Pod")
                    }

                    Button {
                        id: closeActionBtn

                        text: "Close"
                        textColor: "#f87171"
                        onClicked: commandPopup.close()
                    }
                }
            }
        }
    }
}
