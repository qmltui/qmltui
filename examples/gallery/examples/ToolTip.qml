pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property bool pinTip: true
    property int tipDelay: 150
    property int tipTimeout: 4000
    property string lastTriggered: "None"

    readonly property string activeTipText: {
        if (rollbackBtn.activeFocus) {
            return "Rollback cluster to v2.3";
        }
        if (inspectBtn.activeFocus) {
            return "Inspect pod health metrics";
        }
        return "Deploy release v2.4 to prod";
    }

    function togglePin() {
        pinTip = !pinTip;
    }

    function cycleDelay() {
        tipDelay = (tipDelay === 150) ? 450 : 150;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_P) {
                root.togglePin();
                event.accepted = true;
            } else if (event.key === Qt.Key_D) {
                root.cycleDelay();
                event.accepted = true;
            }
        }

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
                    text: "ToolTip Stage"
                    color: "#fda4af"
                }
                Text {
                    text: "[Controls]"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.pinTip ? "PINNED" : "AUTO"
                color: root.pinTip ? "#2cde85" : "#8b949e"
            }
            footer: Text {
                text: "Tab move · P pin · D delay"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "ToolTip"
                color: "#8b949e"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Release Pipeline Toolbar:"
                    color: "#c9d1d9"
                    bold: true
                }

                Row {
                    spacing: 1

                    Button {
                        id: deployBtn

                        focus: true
                        text: "Deploy"
                        textColor: "#2cde85"
                        ToolTip.text: "Deploy release v2.4 to prod"
                        ToolTip.delay: root.tipDelay
                        ToolTip.timeout: root.tipTimeout
                        ToolTip.color: "#161b22"
                        ToolTip.textColor: "#2cde85"
                        ToolTip.visible: root.pinTip && deployBtn.activeFocus
                        onClicked: root.lastTriggered = "Deploy"
                    }

                    Button {
                        id: rollbackBtn

                        text: "Rollback"
                        textColor: "#fbbf24"
                        ToolTip.text: "Rollback cluster to v2.3"
                        ToolTip.delay: root.tipDelay
                        ToolTip.timeout: root.tipTimeout
                        ToolTip.color: "#161b22"
                        ToolTip.textColor: "#fbbf24"
                        ToolTip.visible: root.pinTip && rollbackBtn.activeFocus
                        onClicked: root.lastTriggered = "Rollback"
                    }

                    Button {
                        id: inspectBtn

                        text: "Inspect"
                        textColor: "#38bdf8"
                        ToolTip.text: "Inspect pod health metrics"
                        ToolTip.delay: root.tipDelay
                        ToolTip.timeout: root.tipTimeout
                        ToolTip.color: "#161b22"
                        ToolTip.textColor: "#38bdf8"
                        ToolTip.visible: root.pinTip && inspectBtn.activeFocus
                        onClicked: root.lastTriggered = "Inspect"
                    }
                }

                Text {
                    y: 8
                    text: "Tip: " + root.activeTipText
                    color: "#fda4af"
                    bold: true
                }

                Text {
                    text: "Triggered Action: " + root.lastTriggered
                    color: "#8b949e"
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

                Text {
                    text: "ToolTip.text"
                    color: "#8b949e"
                }

                Text {
                    width: 24
                    text: root.activeTipText
                    color: "#f0f6fc"
                    bold: true
                }

                Text {
                    text: "ToolTip.delay: " + root.tipDelay + "ms"
                    color: "#58a6ff"
                    bold: true
                }

                Text {
                    text: "ToolTip.timeout: " + root.tipTimeout + "ms"
                    color: "#2cde85"
                }

                CheckBox {
                    id: pinCheck

                    text: "Pin Active Tip (P)"
                    checked: root.pinTip
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.pinTip = checked;
                    }
                }
            }
        }
    }
}
