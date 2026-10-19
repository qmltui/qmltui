pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property int targetX: 2
    property int targetWidth: 10
    property int easingIndex: 0
    property bool warmTone: false
    readonly property var easingTypes: [Easing.OutBounce, Easing.OutCubic, Easing.InOutQuad]
    readonly property var easingNames: ["Easing.OutBounce", "Easing.OutCubic", "Easing.InOutQuad"]

    function stepTarget(delta) {
        const maxPos = Math.max(4, trackBox.width - 10);
        let next = root.targetX + delta;
        if (next > maxPos) {
            next = 2;
        } else if (next < 2) {
            next = maxPos;
        }
        root.targetX = next;
        root.targetWidth = root.targetWidth === 10 ? 24 : (root.targetWidth === 24 ? 16 : 10);
        root.warmTone = !root.warmTone;
    }

    function cycleEasing() {
        root.easingIndex = (root.easingIndex + 1) % root.easingTypes.length;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Right || event.key === Qt.Key_Space) {
                root.stepTarget(12);
                event.accepted = true;
            } else if (event.key === Qt.Key_Left) {
                root.stepTarget(-12);
                event.accepted = true;
            } else if (event.key === Qt.Key_E) {
                root.cycleEasing();
                event.accepted = true;
            }
        }

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
                    text: "Behavior"
                    color: "#c084fc"
                }
                Text {
                    text: "[Interceptor]"
                    color: "#2cde85"
                }
            }
            titleTrailing: Text {
                text: root.easingNames[root.easingIndex]
                color: "#58a6ff"
            }
            footer: Text {
                text: "[Left/Right/Space] Move  [E] Easing"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "x=" + puck.x + " w=" + morphBar.width
                color: "#c084fc"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "Behavior on x  ·  " + root.easingNames[root.easingIndex]
                    bold: true
                    color: "#c084fc"
                }

                Rectangle {
                    id: trackBox
                    width: parent.width
                    height: 5
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    Rectangle {
                        id: puck
                        x: root.targetX
                        y: 0
                        width: 8
                        height: 3
                        color: root.warmTone ? "#9a3412" : "#1e3a8a"
                        border {
                            style: Border.Double
                            color: root.warmTone ? "#f97316" : "#58a6ff"
                        }

                        Behavior on x {
                            NumberAnimation {
                                duration: 350
                                easing: root.easingTypes[root.easingIndex]
                            }
                        }

                        Behavior on color {
                            ColorAnimation {
                                duration: 350
                            }
                        }

                        Text {
                            anchors {
                                centerIn: parent
                            }
                            text: "PUCK"
                            bold: true
                            color: "#f0f6fc"
                        }
                    }
                }

                Text {
                    text: "Behavior on width + Behavior on color"
                    bold: true
                    color: "#38bdf8"
                }

                Rectangle {
                    id: morphBar
                    width: root.targetWidth
                    height: 3
                    color: root.warmTone ? "#065f46" : "#4c1d95"
                    border {
                        style: Border.Rounded
                        color: root.warmTone ? "#2cde85" : "#c084fc"
                    }

                    Behavior on width {
                        NumberAnimation {
                            duration: 350
                            easing: root.easingTypes[root.easingIndex]
                        }
                    }

                    Behavior on color {
                        ColorAnimation {
                            duration: 350
                        }
                    }

                    Text {
                        anchors {
                            centerIn: parent
                        }
                        text: "w:" + morphBar.width
                        bold: true
                        color: "#f0f6fc"
                    }
                }

                Text {
                    text: "Target x: " + root.targetX + " (live x: " + puck.x + ")"
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
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "easing:"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.easingNames[root.easingIndex]
                    bold: true
                    color: "#c084fc"
                }

                Text {
                    text: "puck.x: " + puck.x + " -> " + root.targetX
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "bar.width: " + morphBar.width + " -> " + root.targetWidth
                    color: "#2cde85"
                }

                Text {
                    text: "tone: " + (root.warmTone ? "Warm" : "Cool")
                    color: "#f9c74f"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Step Target [Space]"
                    onClicked: root.stepTarget(12)
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Cycle Easing [E]"
                    onClicked: root.cycleEasing()
                }
            }
        }
    }
}
