pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property bool filled: true
    property int sampleCount: 0
    property var cpuValues: [24, 32, 45, 38, 52, 64, 58, 72, 68, 54, 48, 62]
    property var memValues: [40, 42, 44, 45, 48, 50, 52, 53, 55, 56, 58, 60]
    property var latValues: [12, 18, 15, 28, 22, 35, 19, 14, 24, 31, 16, 20]
    property var nextCpuSamples: [81, 74, 88, 65, 59, 92]
    property var nextMemSamples: [63, 66, 68, 70, 72, 75]
    property var nextLatSamples: [42, 29, 51, 18, 25, 38]

    readonly property int cpuLast: cpuValues.length > 0 ? cpuValues[cpuValues.length - 1] : 0
    readonly property int memLast: memValues.length > 0 ? memValues[memValues.length - 1] : 0
    readonly property int latLast: latValues.length > 0 ? latValues[latValues.length - 1] : 0
    readonly property int cpuMin: {
        if (cpuValues.length === 0)
            return 0;
        let m = cpuValues[0];
        for (let i = 1; i < cpuValues.length; ++i) {
            if (cpuValues[i] < m)
                m = cpuValues[i];
        }
        return m;
    }
    readonly property int cpuMax: {
        if (cpuValues.length === 0)
            return 0;
        let m = cpuValues[0];
        for (let i = 1; i < cpuValues.length; ++i) {
            if (cpuValues[i] > m)
                m = cpuValues[i];
        }
        return m;
    }

    function pushSample() {
        const idx = sampleCount % nextCpuSamples.length;
        cpuValues = cpuValues.slice(1).concat([nextCpuSamples[idx]]);
        memValues = memValues.slice(1).concat([nextMemSamples[idx]]);
        latValues = latValues.slice(1).concat([nextLatSamples[idx]]);
        sampleCount += 1;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Space) {
                root.pushSample();
                event.accepted = true;
            } else if (event.key === Qt.Key_F) {
                root.filled = !root.filled;
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
                    text: "Sparkline"
                    color: "#22d3ee"
                }
                Text {
                    text: "Controls"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.filled ? "FILLED" : "OUTLINE"
                color: "#2cde85"
            }
            footer: Text {
                text: "Space push sample  F toggle fill"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "samples: +" + root.sampleCount
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
                    text: "Live Metrics Sparkline Monitor"
                    color: "#c9d1d9"
                    bold: true
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        text: "CPU Load   last: " + root.cpuLast + "%  (min " + root.cpuMin + "% / max " + root.cpuMax + "%)"
                        color: "#22d3ee"
                        bold: true
                    }

                    Sparkline {
                        width: parent.width
                        height: 2
                        from: 0
                        to: 100
                        filled: root.filled
                        values: root.cpuValues
                        color: "#22d3ee"
                    }
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        text: "Memory     last: " + root.memLast + "%"
                        color: "#2cde85"
                        bold: true
                    }

                    Sparkline {
                        width: parent.width
                        height: 1
                        from: 0
                        to: 100
                        filled: root.filled
                        values: root.memValues
                        color: "#2cde85"
                    }
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        text: "Latency    last: " + root.latLast + "ms"
                        color: "#f9c74f"
                        bold: true
                    }

                    Sparkline {
                        width: parent.width
                        height: 2
                        from: 0
                        to: 100
                        filled: root.filled
                        values: root.latValues
                        color: "#f9c74f"
                    }
                }

                Text {
                    text: "Mode: " + (root.filled ? "Filled Area" : "Line Curve") + "  |  Pushed: " + root.sampleCount
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
                anchors {
                    fill: parent
                    leftMargin: 2
                    rightMargin: 2
                    topMargin: 1
                    bottomMargin: 1
                }
                spacing: 1

                Button {
                    focus: true
                    text: "Push Sample"
                    textColor: "#2cde85"

                    onClicked: {
                        root.pushSample();
                    }
                }

                Column {
                    spacing: 0

                    Text {
                        text: "PROPERTIES"
                        color: "#58a6ff"
                        bold: true
                    }

                    CheckBox {
                        checked: root.filled
                        text: "filled"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.filled = checked;
                        }
                    }
                }

                Column {
                    spacing: 0

                    Text {
                        text: "CPU TELEMETRY"
                        color: "#58a6ff"
                        bold: true
                    }

                    Text {
                        text: "min: " + root.cpuMin + "%"
                        color: "#7ee0ff"
                    }

                    Text {
                        text: "max: " + root.cpuMax + "%"
                        color: "#f9c74f"
                    }

                    Text {
                        text: "last: " + root.cpuLast + "%"
                        color: "#22d3ee"
                        bold: true
                    }

                    Text {
                        text: "mem: " + root.memLast + "%"
                        color: "#2cde85"
                    }

                    Text {
                        text: "lat: " + root.latLast + "ms"
                        color: "#f9c74f"
                    }

                    Text {
                        text: "filled: " + (root.filled ? "true" : "false")
                        color: "#8b949e"
                    }
                }
            }
        }
    }
}
