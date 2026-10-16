pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property int progressValue: 58
    property bool showPercentage: true
    property bool showBrackets: true

    function adjustProgress(delta) {
        progressValue = Math.max(0, Math.min(100, progressValue + delta));
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Left || event.key === Qt.Key_Minus) {
                root.adjustProgress(-5);
                event.accepted = true;
            } else if (event.key === Qt.Key_Right || event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
                root.adjustProgress(5);
                event.accepted = true;
            } else if (event.key === Qt.Key_Home) {
                root.progressValue = 0;
                event.accepted = true;
            } else if (event.key === Qt.Key_End) {
                root.progressValue = 100;
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "ProgressBar"
            titleColor: "#38bdf8"
            titleBadge: "Controls"
            titleBadgeColor: "#58a6ff"
            titleTrailing: root.progressValue + "%"
            titleTrailingColor: "#2cde85"
            footer: "Left/Right or +/- adjust"
            footerColor: "#8b949e"
            footerTrailing: "Home/End 0/100"
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
                    text: "Multi-Style Progress Rack (" + root.progressValue + "%)"
                    color: "#c9d1d9"
                    bold: true
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        text: "block (█░)"
                        color: "#38bdf8"
                    }

                    ProgressBar {
                        width: parent.width
                        from: 0
                        to: 100
                        value: root.progressValue
                        style: "block"
                        showBrackets: root.showBrackets
                        showPercentage: root.showPercentage
                        color: "#38bdf8"
                    }

                    Text {
                        text: "segmented (▰▱)"
                        color: "#2cde85"
                    }

                    ProgressBar {
                        width: parent.width
                        from: 0
                        to: 100
                        value: root.progressValue
                        style: "segmented"
                        showBrackets: root.showBrackets
                        showPercentage: root.showPercentage
                        color: "#2cde85"
                    }

                    Text {
                        text: "ascii (#-)"
                        color: "#f9c74f"
                    }

                    ProgressBar {
                        width: parent.width
                        from: 0
                        to: 100
                        value: root.progressValue
                        style: "hash"
                        showBrackets: root.showBrackets
                        showPercentage: root.showPercentage
                        color: "#f9c74f"
                    }

                    Text {
                        text: "horizontal Gradient"
                        color: "#c084fc"
                    }

                    ProgressBar {
                        width: parent.width
                        from: 0
                        to: 100
                        value: root.progressValue
                        style: "block"
                        showBrackets: root.showBrackets
                        showPercentage: root.showPercentage
                        color: "#8b949e"
                        gradient: Gradient {
                            orientation: Gradient.Horizontal

                            GradientStop {
                                position: 0.0
                                color: "#38bdf8"
                            }

                            GradientStop {
                                position: 0.5
                                color: "#2cde85"
                            }

                            GradientStop {
                                position: 1.0
                                color: "#f9c74f"
                            }
                        }
                    }

                    Text {
                        text: "thresholds (60% / 85%)"
                        color: "#f87171"
                    }

                    ProgressBar {
                        width: parent.width
                        from: 0
                        to: 100
                        value: root.progressValue
                        style: "segmented"
                        showBrackets: root.showBrackets
                        showPercentage: root.showPercentage
                        color: "#8b949e"
                        thresholds: [
                            {
                                value: 60,
                                color: "#2cde85"
                            },
                            {
                                value: 85,
                                color: "#f9c74f"
                            },
                            {
                                value: 100,
                                color: "#f87171"
                            }
                        ]
                    }
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
                    text: "ADJUST VALUE"
                    color: "#58a6ff"
                    bold: true
                }

                Row {
                    spacing: 1

                    Button {
                        focus: true
                        text: "-10"
                        textColor: "#f87171"

                        onClicked: {
                            root.adjustProgress(-10);
                        }
                    }

                    Button {
                        text: "+10"
                        textColor: "#2cde85"

                        onClicked: {
                            root.adjustProgress(10);
                        }
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
                        checked: root.showPercentage
                        text: "showPercentage"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.showPercentage = checked;
                        }
                    }

                    CheckBox {
                        checked: root.showBrackets
                        text: "showBrackets"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.showBrackets = checked;
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
                        text: "value: " + root.progressValue + "%"
                        color: "#38bdf8"
                        bold: true
                    }

                    Text {
                        text: "position: " + (root.progressValue / 100).toFixed(2)
                        color: "#2cde85"
                    }

                    Text {
                        text: "brackets: " + (root.showBrackets ? "true" : "false")
                        color: "#c9d1d9"
                    }

                    Text {
                        text: "percent: " + (root.showPercentage ? "true" : "false")
                        color: "#c9d1d9"
                    }
                }
            }
        }
    }
}
