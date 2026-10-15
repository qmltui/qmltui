pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root

    property int reportCount: 0

    function printCapabilities() {
        history.print("--- Host Terminal Capabilities ---");
        history.print("Terminal Program:    " + (root.capabilities.termProgram || "unknown"));
        history.print("Terminal Grid:       " + root.terminalWidth + "x" + root.terminalHeight + " cells");
        history.print("Color Resolution:    " + root.capabilities.colorDepth);
        history.print("Standard Output TTY: " + (root.capabilities.isTty ? "yes" : "no"));
        history.print("Synchronized Output: " + (root.capabilities.syncOutput ? "supported" : "unsupported"));
        history.print("Kitty Keyboard:      " + (root.capabilities.kittyKeyboard ? "supported" : "unsupported"));
        history.print("Kitty Graphics:      " + (root.capabilities.kittyGraphics ? "supported" : "unsupported"));
        history.print("Sixel Graphics:      " + (root.capabilities.sixelGraphics ? "supported" : "unsupported"));
        history.print("Cell Pixel Size:     " + (root.capabilities.cellPixelSize.width > 0 ? (root.capabilities.cellPixelSize.width + "x" + root.capabilities.cellPixelSize.height + " px") : "unknown"));
        history.print("Mouse Reporting:     " + (root.mouseReporting ? "enabled" : "disabled"));
        history.print("-----------------------------------");
        root.reportCount++;
    }

    title: "Terminal Capabilities"
    color: "#0b0f14"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key === Qt.Key_P) {
            root.printCapabilities();
            event.accepted = true;
        }
    }
    Component.onCompleted: root.printCapabilities()

    Scrollback {
        id: history
    }

    Column {
        anchors {
            fill: parent
            margins: 1
        }
        spacing: 1

        Panel {
            width: parent.width
            height: 3
            title: "Terminal Capabilities"
            border {
                color: "#38bdf8"
                style: Border.Single
            }

            Text {
                anchors.centerIn: parent
                color: "#7ee0ff"
                bold: true
                text: "Host Terminal Feature Detection & Diagnostics"
            }
        }

        Row {
            width: parent.width
            spacing: 1

            Panel {
                width: Math.floor((parent.width - 1) / 2)
                height: 16
                title: "Environment & Geometry"
                border {
                    color: "#6e7b8b"
                    style: Border.Single
                }

                Column {
                    anchors {
                        fill: parent
                        margins: 1
                    }
                    spacing: 1

                    Row {
                        spacing: 1
                        Text {
                            width: 18
                            color: "#c9d1d9"
                            text: "Terminal Program:"
                        }
                        Text {
                            color: "#38bdf8"
                            bold: true
                            text: root.capabilities.termProgram || "unknown"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 18
                            color: "#c9d1d9"
                            text: "Grid Dimensions:"
                        }
                        Text {
                            color: "#38bdf8"
                            text: root.terminalWidth + " x " + root.terminalHeight + " cells"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 18
                            color: "#c9d1d9"
                            text: "Color Resolution:"
                        }
                        Text {
                            color: "#65e6a0"
                            bold: true
                            text: root.capabilities.colorDepth
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 18
                            color: "#c9d1d9"
                            text: "Interactive TTY:"
                        }
                        Text {
                            color: root.capabilities.isTty ? "#65e6a0" : "#f87171"
                            text: root.capabilities.isTty ? "Yes" : "No"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 18
                            color: "#c9d1d9"
                            text: "Cell Pixel Size:"
                        }
                        Text {
                            color: root.capabilities.cellPixelSize.width > 0 ? "#65e6a0" : "#6e7b8b"
                            text: root.capabilities.cellPixelSize.width > 0 ? (root.capabilities.cellPixelSize.width + "x" + root.capabilities.cellPixelSize.height + " px") : "not reported"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 18
                            color: "#c9d1d9"
                            text: "Mouse Reporting:"
                        }
                        Text {
                            color: root.mouseReporting ? "#65e6a0" : "#6e7b8b"
                            text: root.mouseReporting ? "Enabled" : "Disabled"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 18
                            color: "#c9d1d9"
                            text: "Clipboard Mode:"
                        }
                        Text {
                            color: "#c9d1d9"
                            text: root.clipboardMode === Terminal.Osc52 ? "OSC 52" : "Disabled"
                        }
                    }
                }
            }

            Panel {
                width: parent.width - Math.floor((parent.width - 1) / 2) - 1
                height: 16
                title: "Protocols & Features"
                border {
                    color: "#6e7b8b"
                    style: Border.Single
                }

                Column {
                    anchors {
                        fill: parent
                        margins: 1
                    }
                    spacing: 1

                    Row {
                        spacing: 1
                        Text {
                            width: 22
                            color: "#c9d1d9"
                            text: "Synchronized Output:"
                        }
                        Text {
                            color: root.capabilities.syncOutput ? "#65e6a0" : "#f87171"
                            bold: true
                            text: root.capabilities.syncOutput ? "[ Supported ]" : "[ Unsupported ]"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 22
                            color: "#c9d1d9"
                            text: "Kitty Keyboard:"
                        }
                        Text {
                            color: root.capabilities.kittyKeyboard ? "#65e6a0" : "#f87171"
                            bold: true
                            text: root.capabilities.kittyKeyboard ? "[ Supported ]" : "[ Unsupported ]"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 22
                            color: "#c9d1d9"
                            text: "Kitty Graphics:"
                        }
                        Text {
                            color: root.capabilities.kittyGraphics ? "#65e6a0" : "#f87171"
                            bold: true
                            text: root.capabilities.kittyGraphics ? "[ Supported ]" : "[ Unsupported ]"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 22
                            color: "#c9d1d9"
                            text: "Sixel Graphics:"
                        }
                        Text {
                            color: root.capabilities.sixelGraphics ? "#65e6a0" : "#f87171"
                            bold: true
                            text: root.capabilities.sixelGraphics ? "[ Supported ]" : "[ Unsupported ]"
                        }
                    }
                    Row {
                        spacing: 1
                        Text {
                            width: 22
                            color: "#c9d1d9"
                            text: "Reports Printed:"
                        }
                        Text {
                            color: "#fbbf24"
                            text: String(root.reportCount)
                        }
                    }
                }
            }
        }

        Row {
            width: parent.width
            height: 3
            spacing: 2

            Button {
                text: "Print Report (P)"
                onClicked: root.printCapabilities()
            }

            Button {
                text: "Quit (Q)"
                onClicked: Qt.quit()
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: "#6e7b8b"
                text: "P: print to scrollback · Q/Esc: exit"
            }
        }
    }
}
