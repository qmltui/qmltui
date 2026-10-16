pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property int alignIndex: 0
    property int wrapIndex: 0
    readonly property int currentAlignment: alignIndex === 0 ? Qt.AlignLeft : (alignIndex === 1 ? Qt.AlignHCenter : Qt.AlignRight)
    readonly property string alignName: alignIndex === 0 ? "Left" : (alignIndex === 1 ? "HCenter" : "Right")
    readonly property int currentWrapMode: wrapIndex === 0 ? Text.WordWrap : (wrapIndex === 1 ? Text.WrapAnywhere : Text.NoWrap)
    readonly property string wrapName: wrapIndex === 0 ? "WordWrap" : (wrapIndex === 1 ? "WrapAnywhere" : "NoWrap")

    function cycleAlign() {
        root.alignIndex = (root.alignIndex + 1) % 3;
    }

    function cycleWrap() {
        root.wrapIndex = (root.wrapIndex + 1) % 3;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_A) {
                root.cycleAlign();
                event.accepted = true;
            } else if (event.key === Qt.Key_W) {
                root.cycleWrap();
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "Text · Typography & Wrapping Lab"
            titleColor: "#f8fafc"
            titleBadge: "[CORE]"
            titleBadgeColor: "#38bdf8"
            titleTrailing: sampleText.implicitWidth + "x" + sampleText.implicitHeight
            titleTrailingColor: "#8b949e"
            footer: "A cycle align · W cycle wrap · Q quit"
            footerColor: "#8b949e"
            footerTrailing: root.alignName + " · " + root.wrapName
            footerTrailingColor: "#2cde85"
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
                    width: parent.width
                    text: "QmlTui Cell Typography & Horizontal Gradient"
                    bold: true
                    gradient: Gradient {
                        orientation: Gradient.Horizontal

                        GradientStop {
                            position: 0.0
                            color: "#2cde85"
                        }

                        GradientStop {
                            position: 0.5
                            color: "#38bdf8"
                        }

                        GradientStop {
                            position: 1.0
                            color: "#c084fc"
                        }
                    }
                }

                Row {
                    spacing: 2

                    Text {
                        text: "bold"
                        bold: true
                        color: "#f8fafc"
                    }

                    Text {
                        text: "italic"
                        italic: true
                        color: "#7ee0ff"
                    }

                    Text {
                        text: "underline"
                        underline: true
                        color: "#fbbf24"
                    }

                    Text {
                        text: "dim"
                        dim: true
                        color: "#8b949e"
                    }

                    Text {
                        text: " reversed "
                        reversed: true
                        color: "#2cde85"
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 8
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    Text {
                        id: sampleText
                        anchors {
                            fill: parent
                            margins: 1
                        }
                        text: "Retained-mode terminal Text measures Unicode graphemes in integer character cells, supporting WordWrap and per-line alignment."
                        color: "#c9d1d9"
                        horizontalAlignment: root.currentAlignment
                        wrapMode: root.currentWrapMode
                    }
                }

                Text {
                    width: parent.width
                    text: "Attributes: bold · italic · underline · dim · reversed"
                    color: "#8b949e"
                }
            }
        }

        Panel {
            Layout.preferredWidth: 26
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            footer: "Text API"
            footerColor: "#8b949e"
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
                    text: "LAYOUT METRICS"
                    bold: true
                    color: "#38bdf8"
                }

                Text {
                    text: "implicitW: " + sampleText.implicitWidth
                    bold: true
                    color: "#f8fafc"
                }

                Text {
                    text: "implicitH: " + sampleText.implicitHeight
                    bold: true
                    color: "#f8fafc"
                }

                Text {
                    text: "viewport:  " + sampleText.width + "x" + sampleText.height
                    color: "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "ALIGNMENT & WRAP"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "Align: " + root.alignName
                    color: "#2cde85"
                }

                Text {
                    text: "Wrap:  " + root.wrapName
                    color: "#fbbf24"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "CONTROLS"
                    bold: true
                    color: "#58a6ff"
                }

                Button {
                    width: parent.width
                    height: 1
                    focus: true
                    text: "[ Align: " + root.alignName + " (A) ]"
                    textColor: "#2cde85"
                    onClicked: root.cycleAlign()
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "[ Wrap: " + root.wrapName + " (W) ]"
                    textColor: "#fbbf24"
                    onClicked: root.cycleWrap()
                }
            }
        }
    }
}
