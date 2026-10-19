pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property int targetX: 12
    property int targetY: 5
    property int clicks: 0
    property int wheelSteps: 0
    property string lastEvent: "Ready"

    function resetTelemetry() {
        root.clicks = 0;
        root.wheelSteps = 0;
        root.targetX = 12;
        root.targetY = 5;
        root.lastEvent = "Reset";
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_R) {
                root.resetTelemetry();
                event.accepted = true;
            } else if (event.key === Qt.Key_Space) {
                root.clicks += 1;
                root.lastEvent = "Key Step";
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
                    text: "MouseArea"
                    color: "#f97316"
                }
                Text {
                    text: padArea.containsMouse ? "[HOVER]" : "[IDLE]"
                    color: padArea.containsMouse ? "#2cde85" : "#8b949e"
                }
            }
            titleTrailing: Text {
                text: "clicks: " + root.clicks
                color: "#58a6ff"
            }
            footer: Text {
                text: "Click / Hover / Scroll inside pad  [R] Reset"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: root.lastEvent
                color: "#f97316"
            }
            Rectangle {
                id: padSurface
                anchors {
                    fill: parent
                    margins: 1
                }
                color: padArea.containsPress ? "#1f2937" : (padArea.containsMouse ? "#131c2e" : "#161b22")
                border {
                    style: padArea.pressed ? Border.Double : Border.Single
                    color: padArea.pressed ? "#f97316" : (padArea.containsMouse ? "#58a6ff" : "#30363d")
                }

                Text {
                    x: 1
                    y: 0
                    text: "Interactive Telemetry Pad (Click, Hover, Wheel)"
                    bold: true
                    color: "#c9d1d9"
                }

                Text {
                    x: 1
                    y: 1
                    text: "Target @ (" + root.targetX + ", " + root.targetY + ")  ·  Wheel: " + root.wheelSteps
                    color: "#8b949e"
                }

                Text {
                    x: Math.max(1, Math.min(padSurface.width - 5, root.targetX))
                    y: Math.max(2, Math.min(padSurface.height - 3, root.targetY))
                    text: padArea.pressed ? "[●]" : "<┼>"
                    bold: true
                    color: padArea.pressed ? "#f97316" : "#2cde85"
                }

                MouseArea {
                    id: padArea
                    anchors {
                        fill: parent
                    }
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    hoverEnabled: true
                    onMouseXChanged: {
                        root.targetX = padArea.mouseX;
                    }
                    onMouseYChanged: {
                        root.targetY = padArea.mouseY;
                    }
                    onClicked: event => {
                        root.clicks += 1;
                        root.targetX = event.x;
                        root.targetY = event.y;
                        root.lastEvent = event.button === Qt.RightButton ? "RightClick" : "LeftClick";
                    }
                    onWheel: event => {
                        root.wheelSteps += event.wheelDeltaY > 0 ? 1 : -1;
                        root.lastEvent = event.wheelDeltaY > 0 ? "WheelUp" : "WheelDown";
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
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "mouseX: " + padArea.mouseX
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "mouseY: " + padArea.mouseY
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "containsMouse: " + padArea.containsMouse
                    color: padArea.containsMouse ? "#2cde85" : "#8b949e"
                }

                Text {
                    text: "pressed: " + padArea.pressed
                    color: padArea.pressed ? "#f97316" : "#8b949e"
                }

                Text {
                    text: "clicks: " + root.clicks
                    bold: true
                    color: "#f9c74f"
                }

                Text {
                    text: "wheelSteps: " + root.wheelSteps
                    color: "#c9d1d9"
                }

                Text {
                    text: "last: " + root.lastEvent
                    color: "#8b949e"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Reset Pad [R]"
                    onClicked: root.resetTelemetry()
                }
            }
        }
    }
}
