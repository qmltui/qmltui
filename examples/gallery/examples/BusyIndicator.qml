pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property bool running: true
    property int interval: 100

    function adjustInterval(delta) {
        interval = Math.max(60, Math.min(200, interval + delta));
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Left || event.key === Qt.Key_Minus) {
                root.adjustInterval(-20);
                event.accepted = true;
            } else if (event.key === Qt.Key_Right || event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
                root.adjustInterval(20);
                event.accepted = true;
            } else if (event.key === Qt.Key_Space) {
                root.running = !root.running;
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
                    text: "BusyIndicator"
                    color: "#f472b6"
                }
                Text {
                    text: "Controls"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.running ? "Running" : "Paused"
                color: root.running ? "#2cde85" : "#f9c74f"
            }
            footer: Text {
                text: "Space run/pause  Left/Right speed"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: root.interval + "ms"
                color: "#7ee0ff"
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
                    text: "Async Pipeline Spinner Monitor"
                    color: "#c9d1d9"
                    bold: true
                }

                Text {
                    text: "Pipeline: " + (root.running ? "Running" : "Paused") + "  |  Cadence: " + root.interval + "ms"
                    color: root.running ? "#2cde85" : "#f9c74f"
                }

                Column {
                    spacing: 1

                    Row {
                        spacing: 2

                        BusyIndicator {
                            id: brailleSpinner

                            running: root.running
                            interval: root.interval
                            color: "#f472b6"
                        }

                        Text {
                            text: "Braille   TLS Handshake & Ingress"
                            color: "#c9d1d9"
                        }
                    }

                    Row {
                        spacing: 2

                        BusyIndicator {
                            id: lineSpinner

                            running: root.running
                            interval: root.interval
                            frames: ["-", "\\", "|", "/"]
                            color: "#38bdf8"
                        }

                        Text {
                            text: "Line      Schema Migration Job"
                            color: "#c9d1d9"
                        }
                    }

                    Row {
                        spacing: 2

                        BusyIndicator {
                            id: boxSpinner

                            running: root.running
                            interval: root.interval
                            frames: ["▖", "▘", "▝", "▗"]
                            color: "#2cde85"
                        }

                        Text {
                            text: "Box       Shard Rebalancer"
                            color: "#c9d1d9"
                        }
                    }

                    Row {
                        spacing: 2

                        BusyIndicator {
                            id: dotsSpinner

                            running: root.running
                            interval: root.interval
                            frames: ["∙∙∙", "●∙∙", "∙●∙", "∙∙●"]
                            color: "#f9c74f"
                        }

                        Text {
                            text: "Dots      Artifact Upload Queue"
                            color: "#c9d1d9"
                        }
                    }

                    Row {
                        spacing: 2

                        BusyIndicator {
                            id: twodotsSpinner

                            running: root.running
                            interval: root.interval * 2
                            frames: ["●", "○"]
                            color: "#f9c74f"
                        }

                        Text {
                            text: "Dots      Artifact Upload Queue"
                            color: "#c9d1d9"
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
                        text: "Glyphs: [" + brailleSpinner.text + "]  [" + lineSpinner.text + "]  [" + boxSpinner.text + "]  [" + dotsSpinner.text + "]"
                        color: "#7ee0ff"
                        bold: true
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
                        focus: true
                        checked: root.running
                        text: "running"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.running = checked;
                        }
                    }
                }

                Row {
                    spacing: 1

                    Button {
                        text: "-20ms"
                        textColor: "#38bdf8"

                        onClicked: {
                            root.adjustInterval(-20);
                        }
                    }

                    Button {
                        text: "+20ms"
                        textColor: "#f9c74f"

                        onClicked: {
                            root.adjustInterval(20);
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
                        text: "state: " + (root.running ? "Running" : "Paused")
                        color: root.running ? "#2cde85" : "#f9c74f"
                        bold: true
                    }

                    Text {
                        text: "interval: " + root.interval + "ms"
                        color: "#7ee0ff"
                    }

                    Text {
                        text: "braille: " + brailleSpinner.text
                        color: "#f472b6"
                    }

                    Text {
                        text: "line: " + lineSpinner.text
                        color: "#38bdf8"
                    }

                    Text {
                        text: "box: " + boxSpinner.text
                        color: "#2cde85"
                    }
                }
            }
        }
    }
}
