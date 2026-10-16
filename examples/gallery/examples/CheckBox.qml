pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property bool controlsEnabled: true
    property int toggleEvents: 0
    property string lastToggled: "None"
    readonly property int activeCount: (tlsBox.checked ? 1 : 0) + (auditBox.checked ? 1 : 0) + (traceBox.checked ? 1 : 0) + (corsBox.checked ? 1 : 0)
    readonly property string cliFlags: {
        let flags = [];
        if (tlsBox.checked)
            flags.push("--tls");
        if (auditBox.checked)
            flags.push("--audit");
        if (traceBox.checked)
            flags.push("--trace");
        if (corsBox.checked)
            flags.push("--cors");
        return flags.length > 0 ? flags.join(" ") : "(none)";
    }
    readonly property string postureLabel: {
        if (activeCount === 4)
            return "HARDENED";
        if (activeCount >= 2)
            return "STANDARD";
        if (activeCount === 1)
            return "MINIMAL";
        return "UNPROTECTED";
    }

    function setAll(state) {
        tlsBox.checked = state;
        auditBox.checked = state;
        traceBox.checked = state;
        corsBox.checked = state;
        root.toggleEvents += 1;
        root.lastToggled = state ? "All" : "None";
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_A) {
                root.setAll(true);
                event.accepted = true;
            } else if (event.key === Qt.Key_N) {
                root.setAll(false);
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "CheckBox"
            titleColor: "#7ee0ff"
            titleBadge: "Controls"
            titleBadgeColor: "#58a6ff"
            titleTrailing: root.postureLabel
            titleTrailingColor: root.activeCount >= 2 ? "#2cde85" : "#f87171"
            footer: "Space toggle  A all  N none"
            footerColor: "#8b949e"
            footerTrailing: "Tab next"
            footerTrailingColor: "#8b949e"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
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
                    text: "Service Security & Logging Policy"
                    color: "#c9d1d9"
                    bold: true
                }

                Column {
                    spacing: 1

                    CheckBox {
                        id: tlsBox

                        focus: true
                        checked: true
                        enabled: root.controlsEnabled
                        hoverEnabled: true
                        text: "TLS 1.3"
                        textColor: checked ? "#7ee0ff" : "#8b949e"

                        onToggled: checked => {
                            root.toggleEvents += 1;
                            root.lastToggled = "TLS 1.3 (" + (checked ? "on" : "off") + ")";
                        }
                    }

                    CheckBox {
                        id: auditBox

                        checked: true
                        enabled: root.controlsEnabled
                        hoverEnabled: true
                        text: "Audit Log"
                        textColor: checked ? "#7ee0ff" : "#8b949e"

                        onToggled: checked => {
                            root.toggleEvents += 1;
                            root.lastToggled = "Audit Log (" + (checked ? "on" : "off") + ")";
                        }
                    }

                    CheckBox {
                        id: traceBox

                        checked: false
                        enabled: root.controlsEnabled
                        hoverEnabled: true
                        text: "Verbose Trace"
                        textColor: checked ? "#7ee0ff" : "#8b949e"

                        onToggled: checked => {
                            root.toggleEvents += 1;
                            root.lastToggled = "Verbose Trace (" + (checked ? "on" : "off") + ")";
                        }
                    }

                    CheckBox {
                        id: corsBox

                        checked: false
                        enabled: root.controlsEnabled
                        hoverEnabled: true
                        text: "Strict CORS"
                        textColor: checked ? "#7ee0ff" : "#8b949e"

                        onToggled: checked => {
                            root.toggleEvents += 1;
                            root.lastToggled = "Strict CORS (" + (checked ? "on" : "off") + ")";
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
                        text: "gateway " + root.cliFlags
                        color: "#2cde85"
                        bold: true
                    }
                }

                Text {
                    text: "Posture: " + root.postureLabel + " (" + root.activeCount + " / 4 enabled)"
                    color: root.activeCount >= 2 ? "#2cde85" : "#f9c74f"
                }
            }
        }

        Panel {
            Layout.preferredWidth: 28
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
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
                    text: "QUICK ACTIONS"
                    color: "#58a6ff"
                    bold: true
                }

                Row {
                    spacing: 1

                    Button {
                        text: "All"
                        textColor: "#2cde85"

                        onClicked: {
                            root.setAll(true);
                        }
                    }

                    Button {
                        text: "None"
                        textColor: "#f87171"

                        onClicked: {
                            root.setAll(false);
                        }
                    }
                }

                CheckBox {
                    checked: root.controlsEnabled
                    text: "enabled"
                    textColor: "#c9d1d9"

                    onToggled: checked => {
                        root.controlsEnabled = checked;
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
                        text: root.activeCount + " / 4 enabled"
                        color: "#7ee0ff"
                        bold: true
                    }

                    Text {
                        text: "CLI flags:"
                        color: "#8b949e"
                    }

                    Text {
                        text: root.cliFlags
                        color: "#2cde85"
                    }

                    Text {
                        text: "events: " + root.toggleEvents
                        color: "#c9d1d9"
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
