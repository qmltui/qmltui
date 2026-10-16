pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property int modeIndex: 0
    property int loadCount: 0

    readonly property string modeName: {
        if (modeIndex === 0) {
            return "SystemCard";
        }
        if (modeIndex === 1) {
            return "NetworkCard";
        }
        return "None (Unloaded)";
    }

    function selectCard(idx) {
        modeIndex = idx;
    }

    Component {
        id: systemCardComp

        Panel {
            title: "SystemCard"
            titleColor: "#2cde85"
            titleTrailing: "ACTIVE"
            titleTrailingColor: "#2cde85"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#2cde85"
            }

            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "CPU Load:   24.8% (16 cores)"
                    color: "#f0f6fc"
                    bold: true
                }

                Text {
                    text: "Memory:     12.4 / 32.0 GiB"
                    color: "#58a6ff"
                }

                Text {
                    text: "Kernel:     Linux 6.12-tui"
                    color: "#8b949e"
                }
            }
        }
    }

    Component {
        id: networkCardComp

        Panel {
            title: "NetworkCard"
            titleColor: "#38bdf8"
            titleTrailing: "ACTIVE"
            titleTrailingColor: "#38bdf8"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#38bdf8"
            }

            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Ingress:    1.42 GB/s"
                    color: "#f0f6fc"
                    bold: true
                }

                Text {
                    text: "Egress:     890 MB/s"
                    color: "#2cde85"
                }

                Text {
                    text: "Sockets:    1,284 TLS active"
                    color: "#8b949e"
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_1) {
                root.selectCard(0);
                event.accepted = true;
            } else if (event.key === Qt.Key_2) {
                root.selectCard(1);
                event.accepted = true;
            } else if (event.key === Qt.Key_3) {
                root.selectCard(2);
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "Loader Workbench"
            titleColor: "#fcd34d"
            titleBadge: "[Core]"
            titleBadgeColor: "#58a6ff"
            titleTrailing: root.modeName
            titleTrailingColor: "#8b949e"
            footer: "1 System · 2 Network · 3 Unload"
            footerColor: "#8b949e"
            footerTrailing: "Loader"
            footerTrailingColor: "#8b949e"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            Row {
                x: 2
                y: 2
                spacing: 1

                Button {
                    id: sysBtn

                    focus: true
                    text: "System"
                    textColor: "#2cde85"
                    onClicked: root.selectCard(0)
                }

                Button {
                    id: netBtn

                    text: "Network"
                    textColor: "#38bdf8"
                    onClicked: root.selectCard(1)
                }

                Button {
                    id: noneBtn

                    text: "Unload"
                    textColor: "#f87171"
                    onClicked: root.selectCard(2)
                }
            }

            Loader {
                id: cardLoader

                x: 2
                y: 6
                width: Math.max(24, parent.width - 4)
                height: 8
                active: root.modeIndex !== 2
                sourceComponent: root.modeIndex === 0 ? systemCardComp : (root.modeIndex === 1 ? networkCardComp : null)
                onLoaded: root.loadCount += 1
            }

            Text {
                x: 4
                y: 9
                visible: cardLoader.status !== Loader.Ready
                text: "None (Unloaded) — 0 delegate items in memory"
                color: "#8b949e"
                bold: true
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
                    text: "loader.active: " + (cardLoader.active ? "true" : "false")
                    color: cardLoader.active ? "#2cde85" : "#f87171"
                    bold: true
                }

                Text {
                    text: "loader.status: " + (cardLoader.status === Loader.Ready ? "Ready" : "Null")
                    color: "#58a6ff"
                    bold: true
                }

                Text {
                    text: "Load Counter: " + root.loadCount
                    color: "#f0f6fc"
                    bold: true
                }

                Text {
                    text: "sourceComponent:"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.modeName
                    color: "#fcd34d"
                }
            }
        }
    }
}
