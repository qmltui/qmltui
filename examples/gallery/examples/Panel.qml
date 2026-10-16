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
            title: "Panel Chrome Stage"
            titleColor: "#2cde85"
            titleBadge: "[Controls]"
            titleBadgeColor: "#58a6ff"
            titleTrailing: root.borderStyleName + " · " + root.alignName
            titleTrailingColor: "#8b949e"
            footer: "B border · A align · G badge · F footer"
            footerColor: "#8b949e"
            footerTrailing: "Esc quit"
            footerTrailingColor: "#8b949e"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            Panel {
                id: previewPanel

                x: 2
                y: 2
                width: Math.max(24, parent.width - 4)
                height: 9
                title: "Service Telemetry"
                titleAlignment: root.alignValue
                titleColor: "#f0f6fc"
                titleBadge: root.showBadge ? "[LIVE]" : ""
                titleBadgeColor: "#2cde85"
                titleTrailing: "v2.4"
                titleTrailingColor: "#58a6ff"
                footer: root.showFooter ? "region: eu-central-1" : ""
                footerColor: "#8b949e"
                footerTrailing: root.showFooter ? "99.98% SLA" : ""
                footerTrailingColor: "#2cde85"
                color: "#161b22"
                border {
                    style: root.borderStyleValue
                    color: "#58a6ff"
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
            title: "Inspector"
            titleColor: "#58a6ff"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
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

                    text: "titleBadge"
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
