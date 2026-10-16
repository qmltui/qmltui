pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property string currentState: "nominal"
    property int transitionCount: 0

    function selectState(name) {
        if (serviceCard.state !== name) {
            root.transitionCount += 1;
            root.currentState = name;
            serviceCard.state = name;
        }
    }

    function cycleState() {
        if (serviceCard.state === "nominal") {
            root.selectState("degraded");
        } else if (serviceCard.state === "degraded") {
            root.selectState("maintenance");
        } else {
            root.selectState("nominal");
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_1) {
                root.selectState("nominal");
                event.accepted = true;
            } else if (event.key === Qt.Key_2) {
                root.selectState("degraded");
                event.accepted = true;
            } else if (event.key === Qt.Key_3) {
                root.selectState("maintenance");
                event.accepted = true;
            } else if (event.key === Qt.Key_Space) {
                root.cycleState();
                event.accepted = true;
            }
        }

        Panel {
            id: stage
            Layout.fillWidth: true
            Layout.fillHeight: true
            focusable: true
            focus: true
            title: "States"
            titleColor: "#34d399"
            titleBadge: "[" + serviceCard.state + "]"
            titleBadgeColor: "#58a6ff"
            titleTrailing: "transitions: " + root.transitionCount
            titleTrailingColor: "#8b949e"
            footer: "[1] nominal  [2] degraded  [3] maintenance"
            footerColor: "#8b949e"
            footerTrailing: "300 ms"
            footerTrailingColor: "#34d399"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "Declarative State Machine + Transition"
                    bold: true
                    color: "#c9d1d9"
                }

                Rectangle {
                    id: serviceCard
                    width: parent.width
                    height: 8
                    color: "#0f291e"
                    border {
                        style: Border.Double
                        color: "#2cde85"
                    }
                    state: "nominal"

                    Column {
                        anchors {
                            fill: parent
                            margins: 1
                        }
                        spacing: 1

                        Text {
                            id: statusTitle
                            text: "NODE: NOMINAL (99.98% SLA)"
                            bold: true
                            color: "#2cde85"
                        }

                        Text {
                            id: statusDetail
                            text: "All 12 replicas healthy · latency 14 ms"
                            color: "#c9d1d9"
                        }

                        Rectangle {
                            id: loadBar
                            width: 10
                            height: 1
                            color: "#2cde85"
                        }

                        Text {
                            text: "Meter width: " + loadBar.width + " cols"
                            color: "#8b949e"
                        }
                    }

                    states: [
                        State {
                            name: "nominal"

                            PropertyChanges {
                                target: serviceCard
                                properties: {
                                    "color": "#0f291e"
                                }
                            }

                            PropertyChanges {
                                target: statusTitle
                                properties: {
                                    "text": "NODE: NOMINAL (99.98% SLA)",
                                    "color": "#2cde85"
                                }
                            }

                            PropertyChanges {
                                target: statusDetail
                                properties: {
                                    "text": "All 12 replicas healthy · latency 14 ms"
                                }
                            }

                            PropertyChanges {
                                target: loadBar
                                properties: {
                                    "width": 10,
                                    "color": "#2cde85"
                                }
                            }
                        },
                        State {
                            name: "degraded"

                            PropertyChanges {
                                target: serviceCard
                                properties: {
                                    "color": "#3b230c"
                                }
                            }

                            PropertyChanges {
                                target: statusTitle
                                properties: {
                                    "text": "NODE: DEGRADED (Elevated Latency)",
                                    "color": "#f9c74f"
                                }
                            }

                            PropertyChanges {
                                target: statusDetail
                                properties: {
                                    "text": "Retry queue active · latency 185 ms"
                                }
                            }

                            PropertyChanges {
                                target: loadBar
                                properties: {
                                    "width": 20,
                                    "color": "#f9c74f"
                                }
                            }
                        },
                        State {
                            name: "maintenance"

                            PropertyChanges {
                                target: serviceCard
                                properties: {
                                    "color": "#1e293b"
                                }
                            }

                            PropertyChanges {
                                target: statusTitle
                                properties: {
                                    "text": "NODE: MAINTENANCE (Draining Traffic)",
                                    "color": "#38bdf8"
                                }
                            }

                            PropertyChanges {
                                target: statusDetail
                                properties: {
                                    "text": "Read-only snapshot in progress"
                                }
                            }

                            PropertyChanges {
                                target: loadBar
                                properties: {
                                    "width": 28,
                                    "color": "#38bdf8"
                                }
                            }
                        }
                    ]

                    transitions: [
                        Transition {
                            from: "*"
                            to: "*"

                            NumberAnimation {
                                duration: 300
                                easing: Easing.OutQuad
                            }

                            ColorAnimation {
                                duration: 300
                            }
                        }
                    ]
                }

                Text {
                    text: "Press 1, 2, 3 or Space to transition between states"
                    color: "#8b949e"
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
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "state: \"" + serviceCard.state + "\""
                    bold: true
                    color: "#34d399"
                }

                Text {
                    text: "bar.width: " + loadBar.width
                    color: "#58a6ff"
                }

                Text {
                    text: "transitions: " + root.transitionCount
                    color: "#c9d1d9"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "1 · Nominal"
                    onClicked: root.selectState("nominal")
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "2 · Degraded"
                    onClicked: root.selectState("degraded")
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "3 · Maintenance"
                    onClicked: root.selectState("maintenance")
                }
            }
        }
    }
}
