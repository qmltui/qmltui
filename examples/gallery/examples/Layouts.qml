pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property int layoutSpacing: 1
    property bool wideSidebar: false

    function cycleSpacing() {
        layoutSpacing = (layoutSpacing + 1) % 3;
    }

    function toggleSidebar() {
        wideSidebar = !wideSidebar;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_G) {
                root.cycleSpacing();
                event.accepted = true;
            } else if (event.key === Qt.Key_S) {
                root.toggleSidebar();
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "Layouts Dashboard Stage"
            titleColor: "#fcd34d"
            titleBadge: "[Core]"
            titleBadgeColor: "#58a6ff"
            titleTrailing: "gap=" + root.layoutSpacing
            titleTrailingColor: "#2cde85"
            footer: "G spacing · S sidebar stretch"
            footerColor: "#8b949e"
            footerTrailing: "Layouts"
            footerTrailingColor: "#8b949e"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            ColumnLayout {
                id: dashColumn

                x: 2
                y: 2
                width: Math.max(24, parent.width - 4)
                height: 9
                spacing: root.layoutSpacing

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: root.layoutSpacing

                    Rectangle {
                        id: navPane

                        Layout.preferredWidth: root.wideSidebar ? 14 : 9
                        Layout.fillHeight: true
                        color: "#161b22"
                        border {
                            style: Border.Single
                            color: "#2cde85"
                        }

                        Text {
                            x: 1
                            y: 1
                            text: "Nav " + navPane.width + "x" + navPane.height
                            color: "#2cde85"
                            bold: true
                        }
                    }

                    GridLayout {
                        id: metricsGrid

                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        columns: 2
                        columnSpacing: root.layoutSpacing
                        rowSpacing: root.layoutSpacing

                        Rectangle {
                            id: cpuCell

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#161b22"
                            border {
                                style: Border.Single
                                color: "#58a6ff"
                            }

                            Text {
                                x: 1
                                y: 1
                                text: "CPU " + cpuCell.width + "x" + cpuCell.height
                                color: "#58a6ff"
                                bold: true
                            }
                        }

                        Rectangle {
                            id: memCell

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#161b22"
                            border {
                                style: Border.Single
                                color: "#fcd34d"
                            }

                            Text {
                                x: 1
                                y: 1
                                text: "MEM " + memCell.width + "x" + memCell.height
                                color: "#fcd34d"
                                bold: true
                            }
                        }

                        Rectangle {
                            id: netCell

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#161b22"
                            border {
                                style: Border.Single
                                color: "#e879f9"
                            }

                            Text {
                                x: 1
                                y: 1
                                text: "NET " + netCell.width + "x" + netCell.height
                                color: "#e879f9"
                            }
                        }

                        Rectangle {
                            id: ioCell

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "#161b22"
                            border {
                                style: Border.Single
                                color: "#38bdf8"
                            }

                            Text {
                                x: 1
                                y: 1
                                text: "I/O " + ioCell.width + "x" + ioCell.height
                                color: "#38bdf8"
                            }
                        }
                    }
                }
            }

            Row {
                x: 2
                y: 12
                spacing: 1

                Button {
                    id: spacingBtn

                    focus: true
                    text: "Spacing (" + root.layoutSpacing + ")"
                    textColor: "#2cde85"
                    onClicked: root.cycleSpacing()
                }

                Button {
                    id: sidebarBtn

                    text: root.wideSidebar ? "Sidebar: Wide" : "Sidebar: Compact"
                    textColor: "#58a6ff"
                    onClicked: root.toggleSidebar()
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
                    text: "spacing: " + root.layoutSpacing
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "sidebarWidth: " + navPane.width
                    color: "#58a6ff"
                    bold: true
                }

                Text {
                    text: "Computed Cells:"
                    color: "#8b949e"
                }

                Text {
                    text: "  Nav:  " + navPane.width + "x" + navPane.height
                    color: "#c9d1d9"
                }

                Text {
                    text: "  Grid: " + metricsGrid.width + "x" + metricsGrid.height
                    color: "#c9d1d9"
                }

                Text {
                    text: "  CPU:  " + cpuCell.width + "x" + cpuCell.height
                    color: "#fcd34d"
                }

                Text {
                    text: "  MEM:  " + memCell.width + "x" + memCell.height
                    color: "#fcd34d"
                }
            }
        }
    }
}
