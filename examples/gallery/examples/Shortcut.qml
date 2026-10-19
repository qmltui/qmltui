pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property bool shortcutsEnabled: true
    property int activationCount: 0
    property string activeProfile: "1: Build"
    property string lastTriggered: "None"

    function triggerShortcut(label) {
        root.activationCount += 1;
        root.activeProfile = label;
        root.lastTriggered = label + " (#" + root.activationCount + ")";
    }

    function clearHistory() {
        root.activationCount = 0;
        root.lastTriggered = "Cleared";
    }

    Shortcut {
        sequence: Qt.Key_1
        enabled: root.shortcutsEnabled
        onActivated: root.triggerShortcut("1: Build")
    }

    Shortcut {
        sequence: Qt.Key_2
        enabled: root.shortcutsEnabled
        onActivated: root.triggerShortcut("2: Test")
    }

    Shortcut {
        sequence: Qt.Key_3
        enabled: root.shortcutsEnabled
        onActivated: root.triggerShortcut("3: Deploy")
    }

    Shortcut {
        sequence: Qt.Key_C
        enabled: root.shortcutsEnabled
        onActivated: root.clearHistory()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Panel {
            id: stage
            Layout.fillWidth: true
            Layout.fillHeight: true
            focusable: true
            focus: true
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            title: Row {
                spacing: 1
                Text {
                    text: "Shortcut"
                    color: "#fb7185"
                }
                Text {
                    text: root.shortcutsEnabled ? "[ENABLED]" : "[MUTED]"
                    color: root.shortcutsEnabled ? "#2cde85" : "#f87171"
                }
            }
            titleTrailing: Text {
                text: "hits: " + root.activationCount
                color: "#8b949e"
            }
            footer: Text {
                text: "[1] Build  [2] Test  [3] Deploy  [C] Clear"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: root.activeProfile
                color: "#fb7185"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "Global Key Router (fires without focus)"
                    bold: true
                    color: "#c9d1d9"
                }

                Rectangle {
                    width: parent.width
                    height: 3
                    color: root.activeProfile === "1: Build" ? "#12261e" : "#161b22"
                    border {
                        style: root.activeProfile === "1: Build" ? Border.Double : Border.Single
                        color: root.activeProfile === "1: Build" ? "#2cde85" : "#30363d"
                    }

                    Text {
                        x: 1
                        text: "[Key 1]  Build Pipeline   -> " + (root.activeProfile === "1: Build" ? "ACTIVE" : "standby")
                        bold: root.activeProfile === "1: Build"
                        color: root.activeProfile === "1: Build" ? "#2cde85" : "#8b949e"
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 3
                    color: root.activeProfile === "2: Test" ? "#12261e" : "#161b22"
                    border {
                        style: root.activeProfile === "2: Test" ? Border.Double : Border.Single
                        color: root.activeProfile === "2: Test" ? "#58a6ff" : "#30363d"
                    }

                    Text {
                        x: 1
                        text: "[Key 2]  Test Suite       -> " + (root.activeProfile === "2: Test" ? "ACTIVE" : "standby")
                        bold: root.activeProfile === "2: Test"
                        color: root.activeProfile === "2: Test" ? "#58a6ff" : "#8b949e"
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 3
                    color: root.activeProfile === "3: Deploy" ? "#12261e" : "#161b22"
                    border {
                        style: root.activeProfile === "3: Deploy" ? Border.Double : Border.Single
                        color: root.activeProfile === "3: Deploy" ? "#fb7185" : "#30363d"
                    }

                    Text {
                        x: 1
                        text: "[Key 3]  Deploy Release   -> " + (root.activeProfile === "3: Deploy" ? "ACTIVE" : "standby")
                        bold: root.activeProfile === "3: Deploy"
                        color: root.activeProfile === "3: Deploy" ? "#fb7185" : "#8b949e"
                    }
                }

                Text {
                    text: "Last Action: " + root.lastTriggered
                    bold: true
                    color: "#f9c74f"
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
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "enabled: " + root.shortcutsEnabled
                    bold: true
                    color: root.shortcutsEnabled ? "#2cde85" : "#f87171"
                }

                Text {
                    text: "sequences: 1, 2, 3, C"
                    color: "#c9d1d9"
                }

                Text {
                    text: "activations: " + root.activationCount
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "route: " + root.activeProfile
                    color: "#fb7185"
                }

                Text {
                    text: "history: " + root.lastTriggered
                    color: "#8b949e"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: root.shortcutsEnabled ? "Mute Shortcuts" : "Enable Shortcuts"
                    onClicked: root.shortcutsEnabled = !root.shortcutsEnabled
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Clear Count [C]"
                    onClicked: root.clearHistory()
                }
            }
        }
    }
}
