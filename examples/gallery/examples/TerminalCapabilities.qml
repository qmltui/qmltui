pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property int reportCount: 0
    readonly property Terminal term: hostTerminal()

    function hostTerminal(): Terminal {
        let p = parent;
        while (p && p.capabilities === undefined)
            p = p.parent;
        return p as Terminal;
    }

    function printCapabilities() {
        const t = root.term;
        history.print("--- Host Terminal Capabilities ---");
        history.print("Terminal Program:    " + (t && t.capabilities.termProgram ? t.capabilities.termProgram : "unknown"));
        history.print("Terminal Grid:       " + (t ? t.terminalWidth : 0) + "x" + (t ? t.terminalHeight : 0) + " cells");
        history.print("Color Resolution:    " + (t ? t.capabilities.colorDepth : "unknown"));
        history.print("Standard Output TTY: " + (t && t.capabilities.isTty ? "yes" : "no"));
        history.print("Synchronized Output: " + (t && t.capabilities.syncOutput ? "supported" : "unsupported"));
        history.print("Kitty Keyboard:      " + (t && t.capabilities.kittyKeyboard ? "supported" : "unsupported"));
        history.print("Kitty Graphics:      " + (t && t.capabilities.kittyGraphics ? "supported" : "unsupported"));
        history.print("Sixel Graphics:      " + (t && t.capabilities.sixelGraphics ? "supported" : "unsupported"));
        history.print("Cell Pixel Size:     " + (t && t.capabilities.cellPixelSize.width > 0 ? (t.capabilities.cellPixelSize.width + "x" + t.capabilities.cellPixelSize.height + " px") : "unknown"));
        history.print("Mouse Reporting:     " + (t && t.mouseReporting ? "enabled" : "disabled"));
        history.print("-----------------------------------");
        root.reportCount += 1;
    }

    Component.onCompleted: root.printCapabilities()

    Keys.onPressed: event => {
        if (event.key === Qt.Key_P) {
            root.printCapabilities();
            event.accepted = true;
        }
    }

    Scrollback {
        id: history
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Panel {
            Layout.fillWidth: true
            Layout.preferredHeight: 3
            border {
                color: "#38bdf8"
                style: Border.Single
            }

            title: Row {
                spacing: 1
                Text {
                    text: "Terminal Capabilities"
                    color: "#38bdf8"
                }
                Text {
                    text: "[Core]"
                    color: "#2cde85"
                }
            }
            Text {
                anchors.centerIn: parent
                color: "#7ee0ff"
                bold: true
                text: "Host Terminal Feature Detection & Diagnostics"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 1

            Panel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                border {
                    color: "#30363d"
                    style: Border.Single
                }

                title: Text {
                    text: "Environment & Geometry"
                    color: "#38bdf8"
                }
                Column {
                    anchors {
                        fill: parent
                        margins: 1
                    }
                    spacing: 0

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
                            text: (root.term && root.term.capabilities.termProgram) ? root.term.capabilities.termProgram : "unknown"
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
                            text: (root.term ? root.term.terminalWidth : 0) + " x " + (root.term ? root.term.terminalHeight : 0) + " cells"
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
                            color: "#2cde85"
                            bold: true
                            text: root.term ? root.term.capabilities.colorDepth : "unknown"
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
                            color: (root.term && root.term.capabilities.isTty) ? "#2cde85" : "#f87171"
                            text: (root.term && root.term.capabilities.isTty) ? "Yes" : "No"
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
                            color: (root.term && root.term.capabilities.cellPixelSize.width > 0) ? "#2cde85" : "#8b949e"
                            text: (root.term && root.term.capabilities.cellPixelSize.width > 0) ? (root.term.capabilities.cellPixelSize.width + "x" + root.term.capabilities.cellPixelSize.height + " px") : "not reported"
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
                            color: (root.term && root.term.mouseReporting) ? "#2cde85" : "#8b949e"
                            text: (root.term && root.term.mouseReporting) ? "Enabled" : "Disabled"
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
                            text: (root.term && root.term.clipboardMode === Terminal.Osc52) ? "OSC 52" : "Disabled"
                        }
                    }
                }
            }

            Panel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                border {
                    color: "#30363d"
                    style: Border.Single
                }

                title: Text {
                    text: "Protocols & Features"
                    color: "#38bdf8"
                }
                Column {
                    anchors {
                        fill: parent
                        margins: 1
                    }
                    spacing: 0

                    Row {
                        spacing: 1

                        Text {
                            width: 21
                            color: "#c9d1d9"
                            text: "Synchronized Output:"
                        }

                        Text {
                            color: (root.term && root.term.capabilities.syncOutput) ? "#2cde85" : "#f87171"
                            bold: true
                            text: (root.term && root.term.capabilities.syncOutput) ? "[ Supported ]" : "[ Unsupported ]"
                        }
                    }

                    Row {
                        spacing: 1

                        Text {
                            width: 21
                            color: "#c9d1d9"
                            text: "Kitty Keyboard:"
                        }

                        Text {
                            color: (root.term && root.term.capabilities.kittyKeyboard) ? "#2cde85" : "#f87171"
                            bold: true
                            text: (root.term && root.term.capabilities.kittyKeyboard) ? "[ Supported ]" : "[ Unsupported ]"
                        }
                    }

                    Row {
                        spacing: 1

                        Text {
                            width: 21
                            color: "#c9d1d9"
                            text: "Kitty Graphics:"
                        }

                        Text {
                            color: (root.term && root.term.capabilities.kittyGraphics) ? "#2cde85" : "#f87171"
                            bold: true
                            text: (root.term && root.term.capabilities.kittyGraphics) ? "[ Supported ]" : "[ Unsupported ]"
                        }
                    }

                    Row {
                        spacing: 1

                        Text {
                            width: 21
                            color: "#c9d1d9"
                            text: "Sixel Graphics:"
                        }

                        Text {
                            color: (root.term && root.term.capabilities.sixelGraphics) ? "#2cde85" : "#f87171"
                            bold: true
                            text: (root.term && root.term.capabilities.sixelGraphics) ? "[ Supported ]" : "[ Unsupported ]"
                        }
                    }

                    Row {
                        spacing: 1

                        Text {
                            width: 21
                            color: "#c9d1d9"
                            text: "Reports Printed:"
                        }

                        Text {
                            color: "#fbbf24"
                            bold: true
                            text: String(root.reportCount)
                        }
                    }
                }
            }
        }

        Row {
            Layout.fillWidth: true
            Layout.preferredHeight: 3
            spacing: 2

            Button {
                focus: true
                text: "Print Report (P)"
                textColor: "#38bdf8"
                onClicked: root.printCapabilities()
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                color: "#8b949e"
                text: "P: print to Scrollback · Esc: back"
            }
        }
    }
}
