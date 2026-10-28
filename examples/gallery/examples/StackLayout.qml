pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// StackLayout: Indexed page container showing one child item at a time.
Column {
    anchors.centerIn: parent
    spacing: 1

    Row {
        spacing: 2

        Button {
            text: "◀ Prev"
            focus: true
            onClicked: stack.currentIndex = (stack.currentIndex + 2) % 3
        }

        Button {
            text: "Next ▶"
            onClicked: stack.currentIndex = (stack.currentIndex + 1) % 3
        }

        Text {
            y: 1
            text: "Page " + (stack.currentIndex + 1) + " of " + stack.count
            color: "#8b949e"
        }
    }

    StackLayout {
        id: stack
        width: 46
        height: 7
        currentIndex: 0

        Panel {
            title: Text {
                text: "Page 1: System Overview"
                color: "#58a6ff"
                bold: true
            }
            border {
                style: Border.Rounded
                color: "#58a6ff"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }

                Text {
                    text: "Cluster Status: Healthy"
                    color: "#2cde85"
                }

                Text {
                    text: "Active Nodes: 12 / 12 online"
                    color: "#c9d1d9"
                }
            }
        }

        Panel {
            title: Text {
                text: "Page 2: Network Configuration"
                color: "#2cde85"
                bold: true
            }
            border {
                style: Border.Rounded
                color: "#2cde85"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }

                Text {
                    text: "Interface: eth0 (10.0.4.15/24)"
                    color: "#c9d1d9"
                }

                Text {
                    text: "Gateway: 10.0.4.1 (connected)"
                    color: "#8b949e"
                }
            }
        }

        Panel {
            title: Text {
                text: "Page 3: Storage Volumes"
                color: "#f2cc60"
                bold: true
            }
            border {
                style: Border.Rounded
                color: "#f2cc60"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }

                Text {
                    text: "/dev/nvme0n1p2: 412 GB free"
                    color: "#c9d1d9"
                }

                Text {
                    text: "Last scrub: 0 errors found"
                    color: "#8b949e"
                }
            }
        }
    }
}
