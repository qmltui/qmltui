pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property bool switchesEnabled: true
    property string lastToggled: "None"
    readonly property int onlineCount: (dbSwitch.checked ? 1 : 0) + (cacheSwitch.checked ? 1 : 0) + (workersSwitch.checked ? 1 : 0) + (telemetrySwitch.checked ? 1 : 0)
    readonly property int memoryFootprintMb: (dbSwitch.checked ? 512 : 0) + (cacheSwitch.checked ? 256 : 0) + (workersSwitch.checked ? 384 : 0) + (telemetrySwitch.checked ? 128 : 0)
    readonly property string healthBadge: {
        if (!dbSwitch.checked)
            return "DEGRADED";
        if (onlineCount === 4)
            return "OPTIMAL";
        if (onlineCount >= 2)
            return "HEALTHY";
        return "LIMITED";
    }
    readonly property string healthColor: {
        if (!dbSwitch.checked)
            return "#f87171";
        if (onlineCount === 4)
            return "#2cde85";
        return "#f9c74f";
    }

    function setAllPower(on) {
        dbSwitch.checked = on;
        cacheSwitch.checked = on;
        workersSwitch.checked = on;
        telemetrySwitch.checked = on;
        root.lastToggled = on ? "All Online" : "All Offline";
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
                    text: "Switch"
                    color: "#ff9f6e"
                }
                Text {
                    text: "Controls"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.healthBadge
                color: root.healthColor
            }
            footer: Text {
                text: "Space toggle power"
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
                    text: "Cluster Subsystem Power Switches"
                    color: "#c9d1d9"
                    bold: true
                }

                Column {
                    spacing: 1

                    Switch {
                        id: dbSwitch

                        focus: true
                        checked: true
                        enabled: root.switchesEnabled
                        hoverEnabled: true
                        text: "Primary Database"
                        textColor: checked ? "#2cde85" : "#8b949e"

                        onToggled: checked => {
                            root.lastToggled = "Database: " + (checked ? "ON" : "OFF");
                        }
                    }

                    Switch {
                        id: cacheSwitch

                        checked: true
                        enabled: root.switchesEnabled
                        hoverEnabled: true
                        text: "Redis Cache"
                        textColor: checked ? "#2cde85" : "#8b949e"

                        onToggled: checked => {
                            root.lastToggled = "Redis: " + (checked ? "ON" : "OFF");
                        }
                    }

                    Switch {
                        id: workersSwitch

                        checked: true
                        enabled: root.switchesEnabled
                        hoverEnabled: true
                        text: "Background Workers"
                        textColor: checked ? "#2cde85" : "#8b949e"

                        onToggled: checked => {
                            root.lastToggled = "Workers: " + (checked ? "ON" : "OFF");
                        }
                    }

                    Switch {
                        id: telemetrySwitch

                        checked: false
                        enabled: root.switchesEnabled
                        hoverEnabled: true
                        text: "Telemetry Exporter"
                        textColor: checked ? "#2cde85" : "#8b949e"

                        onToggled: checked => {
                            root.lastToggled = "Telemetry: " + (checked ? "ON" : "OFF");
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 3
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    Text {
                        anchors {
                            verticalCenter: parent.verticalCenter
                            left: parent.left
                            leftMargin: 2
                        }
                        text: "RAM: " + root.memoryFootprintMb + " MB  |  Health: " + root.healthBadge
                        color: root.healthColor
                        bold: true
                    }
                }

                Text {
                    text: "Services: " + root.onlineCount + " / 4 online  |  Last: " + root.lastToggled
                    color: "#c9d1d9"
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

                Text {
                    text: "POWER CONTROLS"
                    color: "#58a6ff"
                    bold: true
                }

                Row {
                    spacing: 1

                    Button {
                        text: "All On"
                        textColor: "#2cde85"

                        onClicked: {
                            root.setAllPower(true);
                        }
                    }

                    Button {
                        text: "Kill"
                        textColor: "#f87171"

                        onClicked: {
                            root.setAllPower(false);
                        }
                    }
                }

                CheckBox {
                    checked: root.switchesEnabled
                    text: "enabled"
                    textColor: "#c9d1d9"

                    onToggled: checked => {
                        root.switchesEnabled = checked;
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
                        text: "online: " + root.onlineCount + " / 4"
                        color: "#2cde85"
                        bold: true
                    }

                    Text {
                        text: "memory: " + root.memoryFootprintMb + " MB"
                        color: "#7ee0ff"
                    }

                    Text {
                        text: "health: " + root.healthBadge
                        color: root.healthColor
                    }

                    Text {
                        text: "last: " + root.lastToggled
                        color: "#c9d1d9"
                    }
                }
            }
        }
    }
}
