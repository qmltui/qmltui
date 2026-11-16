pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Panel {
        id: root
        anchors.centerIn: parent
        width: 64
        height: 19
        border {
            style: Border.Single
            color: "gray"
        }
        title: Text {
            text: "QmlTui StatusBadge Component Demo"
            color: "cyan"
            bold: true
        }
        footer: Text {
            text: "Press Q or Escape to exit"
            dim: true
        }

        Column {
            anchors {
                fill: parent
                leftMargin: 1
                rightMargin: 1
            }
            spacing: 1

            Text {
                text: "Subtle Variant (Default):"
                bold: true
            }
            Row {
                spacing: 2
                StatusBadge {
                    status: "success"
                }
                StatusBadge {
                    status: "warning"
                }
                StatusBadge {
                    status: "error"
                }
                StatusBadge {
                    status: "info"
                }
            }

            Text {
                text: "Custom Labels and Glyphs:"
                bold: true
            }
            Row {
                spacing: 2
                StatusBadge {
                    status: "success"
                    text: "ONLINE"
                    icon: "●"
                }
                StatusBadge {
                    status: "warning"
                    text: "HIGH LOAD"
                    icon: "▲"
                }
                StatusBadge {
                    status: "error"
                    text: "CRITICAL"
                    icon: "✖"
                }
                StatusBadge {
                    status: "info"
                    text: "PENDING"
                    icon: "…"
                }
            }

            Text {
                text: "Outlined Variant:"
                bold: true
            }
            Row {
                spacing: 2
                StatusBadge {
                    variant: "outlined"
                    status: "success"
                    text: "SYSTEM OK"
                }
                StatusBadge {
                    variant: "outlined"
                    status: "warning"
                    text: "DISK 92%"
                }
                StatusBadge {
                    variant: "outlined"
                    status: "error"
                    text: "DB DOWN"
                }
            }

            Text {
                text: "Filled Variant:"
                bold: true
            }
            Row {
                spacing: 2
                StatusBadge {
                    variant: "filled"
                    status: "success"
                    text: " ACTIVE "
                }
                StatusBadge {
                    variant: "filled"
                    status: "warning"
                    text: " PAUSED "
                }
                StatusBadge {
                    variant: "filled"
                    status: "error"
                    text: " FAILED "
                }
                StatusBadge {
                    variant: "filled"
                    status: "info"
                    text: " BUSY "
                }
            }
        }
    }
}
