pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property bool forwardPass: true
    property int runCount: 1
    readonly property int maxBarWidth: 28

    function triggerRun() {
        root.forwardPass = !root.forwardPass;
        root.runCount += 1;
        linearAnim.stop();
        quadAnim.stop();
        cubicAnim.stop();
        bounceAnim.stop();
        colorAnim.stop();
        linearAnim.start();
        quadAnim.start();
        cubicAnim.start();
        bounceAnim.start();
        colorAnim.start();
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_Space || event.key === Qt.Key_R) {
                root.triggerRun();
                event.accepted = true;
            }
        }

        Panel {
            id: stage
            Layout.fillWidth: true
            Layout.fillHeight: true
            focusable: true
            focus: true
            title: "Animation"
            titleColor: "#f9a8d4"
            titleBadge: linearAnim.running ? "[RUNNING]" : "[IDLE]"
            titleBadgeColor: linearAnim.running ? "#2cde85" : "#8b949e"
            titleTrailing: "run #" + root.runCount
            titleTrailingColor: "#8b949e"
            footer: "[Space] Toggle / Restart Timeline"
            footerColor: "#8b949e"
            footerTrailing: "500 ms"
            footerTrailingColor: "#58a6ff"
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
                spacing: 0

                Text {
                    text: "Easing.Linear (" + linearBar.width + " cols)"
                    color: "#8b949e"
                }

                Rectangle {
                    id: linearBar
                    width: 6
                    height: 1
                    color: "#38bdf8"

                    NumberAnimation {
                        id: linearAnim
                        target: linearBar
                        property: "width"
                        from: root.forwardPass ? 6 : root.maxBarWidth
                        to: root.forwardPass ? root.maxBarWidth : 6
                        duration: 500
                        easing: Easing.Linear
                        running: true
                    }
                }

                Text {
                    text: "Easing.OutQuad (" + quadBar.width + " cols)"
                    color: "#8b949e"
                }

                Rectangle {
                    id: quadBar
                    width: 6
                    height: 1
                    color: "#2cde85"

                    NumberAnimation {
                        id: quadAnim
                        target: quadBar
                        property: "width"
                        from: root.forwardPass ? 6 : root.maxBarWidth
                        to: root.forwardPass ? root.maxBarWidth : 6
                        duration: 500
                        easing: Easing.OutQuad
                        running: true
                    }
                }

                Text {
                    text: "Easing.InOutCubic (" + cubicBar.width + " cols)"
                    color: "#8b949e"
                }

                Rectangle {
                    id: cubicBar
                    width: 6
                    height: 1
                    color: "#f9c74f"

                    NumberAnimation {
                        id: cubicAnim
                        target: cubicBar
                        property: "width"
                        from: root.forwardPass ? 6 : root.maxBarWidth
                        to: root.forwardPass ? root.maxBarWidth : 6
                        duration: 500
                        easing: Easing.InOutCubic
                        running: true
                    }
                }

                Text {
                    text: "Easing.OutBounce (" + bounceBar.width + " cols)"
                    color: "#8b949e"
                }

                Rectangle {
                    id: bounceBar
                    width: 6
                    height: 1
                    color: "#f472b6"

                    NumberAnimation {
                        id: bounceAnim
                        target: bounceBar
                        property: "width"
                        from: root.forwardPass ? 6 : root.maxBarWidth
                        to: root.forwardPass ? root.maxBarWidth : 6
                        duration: 500
                        easing: Easing.OutBounce
                        running: true
                    }
                }

                Rectangle {
                    id: colorSwatch
                    width: parent.width
                    height: 3
                    color: "#1e3a8a"
                    border {
                        style: Border.Single
                        color: "#58a6ff"
                    }

                    ColorAnimation {
                        id: colorAnim
                        target: colorSwatch
                        property: "color"
                        from: root.forwardPass ? "#1e3a8a" : "#065f46"
                        to: root.forwardPass ? "#065f46" : "#1e3a8a"
                        duration: 500
                        running: true
                    }

                    Text {
                        anchors {
                            centerIn: parent
                        }
                        text: "ColorAnimation Swatch"
                        bold: true
                        color: "#f0f6fc"
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
                    margins: 1
                }
                spacing: 0

                Text {
                    text: "running: " + linearAnim.running
                    bold: true
                    color: linearAnim.running ? "#2cde85" : "#8b949e"
                }

                Text {
                    text: "direction: " + (root.forwardPass ? "Expand" : "Contract")
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "duration: 500 ms"
                    color: "#c9d1d9"
                }

                Text {
                    text: "Linear:     " + linearBar.width + " / " + root.maxBarWidth
                    color: "#38bdf8"
                }

                Text {
                    text: "OutQuad:    " + quadBar.width + " / " + root.maxBarWidth
                    color: "#2cde85"
                }

                Text {
                    text: "InOutCubic: " + cubicBar.width + " / " + root.maxBarWidth
                    color: "#f9c74f"
                }

                Text {
                    text: "OutBounce:  " + bounceBar.width + " / " + root.maxBarWidth
                    color: "#f472b6"
                }

                Text {
                    text: "runs: " + root.runCount
                    color: "#8b949e"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Play / Flip [Space]"
                    onClicked: root.triggerRun()
                }
            }
        }
    }
}
