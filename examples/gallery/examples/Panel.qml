pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property int borderIndex: 0
    property int alignIndex: 0
    property bool showBadge: true
    property bool showFooter: true

    readonly property var borderStyles: [Border.Rounded, Border.Single, Border.Double, Border.Ascii]
    readonly property var borderNames: ["Rounded", "Single", "Double", "Ascii"]
    readonly property var alignValues: [Qt.AlignLeft, Qt.AlignHCenter, Qt.AlignRight]
    readonly property var alignNames: ["Left", "Center", "Right"]

    readonly property int borderStyleValue: borderStyles[borderIndex]
    readonly property string borderStyleName: borderNames[borderIndex]
    readonly property int alignValue: alignValues[alignIndex]
    readonly property string alignName: alignNames[alignIndex]

    function cycleBorder() {
        borderIndex = (borderIndex + 1) % borderStyles.length;
    }

    function cycleAlign() {
        alignIndex = (alignIndex + 1) % alignValues.length;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_B) {
                root.cycleBorder();
                event.accepted = true;
            } else if (event.key === Qt.Key_A) {
                root.cycleAlign();
                event.accepted = true;
            } else if (event.key === Qt.Key_G) {
                root.showBadge = !root.showBadge;
                event.accepted = true;
            } else if (event.key === Qt.Key_F) {
                root.showFooter = !root.showFooter;
                event.accepted = true;
            }
        }

        Panel {
            id: stagePanel

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
                    text: "Panel Chrome Stage"
                    color: "#2cde85"
                }

                Text {
                    text: "[Controls]"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.borderStyleName + " · " + root.alignName
                color: "#8b949e"
            }
            footer: Text {
                text: "B border · A align · G badge · F footer"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "Esc quit"
                color: "#8b949e"
            }

            Panel {
                id: previewPanel

                x: 2
                y: 2
                width: Math.max(24, parent.width - 4)
                height: 9
                titleAlignment: root.alignValue
                color: "#161b22"
                border {
                    style: root.borderStyleValue
                    color: "#58a6ff"
                }
                title: Row {
                    spacing: 1

                    Text {
                        text: "Service Telemetry"
                        color: "#f0f6fc"
                    }

                    Text {
                        visible: root.showBadge
                        text: "[LIVE]"
                        color: "#2cde85"

                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.showBadge = !root.showBadge
                        }
                    }
                }
                titleTrailing: Text {
                    text: "v2.4"
                    color: "#58a6ff"
                }
                footer: Text {
                    visible: root.showFooter
                    text: "region: eu-central-1"
                    color: "#8b949e"

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.cycleBorder()
                    }
                }
                footerTrailing: Text {
                    visible: root.showFooter
                    text: "99.98% SLA"
                    color: "#2cde85"
                }

                Column {
                    x: 2
                    y: 2
                    spacing: 1

                    Text {
                        text: "Border Style: " + root.borderStyleName
                        color: "#f0f6fc"
                        bold: true
                    }

                    Text {
                        text: "Title Align:  " + root.alignName
                        color: "#58a6ff"
                    }

                    Text {
                        text: "Chrome Flags: " + (root.showBadge ? "badge " : "no-badge ") + (root.showFooter ? "footer" : "no-footer")
                        color: "#8b949e"
                    }
                }
            }

            Row {
                x: 2
                y: 12
                spacing: 1

                Button {
                    id: borderBtn

                    focus: true
                    text: "Border (B)"
                    textColor: "#2cde85"
                    onClicked: root.cycleBorder()
                }

                Button {
                    id: alignBtn

                    text: "Align (A)"
                    textColor: "#58a6ff"
                    onClicked: root.cycleAlign()
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
                    text: "border.style"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.borderStyleName
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "titleAlignment"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.alignName
                    color: "#58a6ff"
                    bold: true
                }

                CheckBox {
                    id: badgeCheck

                    text: "badge"
                    checked: root.showBadge
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.showBadge = checked;
                    }
                }

                CheckBox {
                    id: footerCheck

                    text: "footer"
                    checked: root.showFooter
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.showFooter = checked;
                    }
                }

                Text {
                    text: "Size: " + previewPanel.width + "x" + previewPanel.height
                    color: "#8b949e"
                }
            }
        }
    }
}
