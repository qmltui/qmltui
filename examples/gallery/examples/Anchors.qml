pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// Anchors: Relative edge attachment (AnchorLine), centering, and margins.
Column {
    id: anchorsDemo

    property bool wideSidebar: false

    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "Items attach their edges (top, bottom, left, right) to sibling or parent AnchorLines."
        color: "#8b949e"
    }

    Item {
        width: 56
        height: 11

        // Top header bar anchored to parent left, top, and right
        Rectangle {
            id: header
            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
            }
            height: 3
            border {
                style: Border.Single
                color: "#58a6ff"
            }

            Text {
                anchors.centerIn: parent
                text: "Header (top, left, right → parent)"
                color: "#58a6ff"
                bold: true
            }
        }

        // Left sidebar anchored below header and to parent bottom
        Rectangle {
            id: sidebar
            anchors {
                top: header.bottom
                bottom: parent.bottom
                left: parent.left
            }
            width: anchorsDemo.wideSidebar ? 24 : 16
            border {
                style: Border.Single
                color: "#f2cc60"
            }

            Text {
                anchors.centerIn: parent
                text: "Sidebar (" + sidebar.width + ")"
                color: "#f2cc60"
            }
        }

        // Main pane stretches dynamically between sidebar.right and parent.right
        Rectangle {
            anchors {
                top: header.bottom
                bottom: parent.bottom
                left: sidebar.right
                right: parent.right
                leftMargin: 1
            }
            border {
                style: Border.Rounded
                color: "#2cde85"
            }

            Column {
                anchors.centerIn: parent

                Text {
                    text: "Content (left: sidebar.right)"
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "Width auto-resizes to " + (56 - sidebar.width - 1) + " cols"
                    color: "#8b949e"
                }
            }
        }
    }

    Button {
        text: anchorsDemo.wideSidebar ? "Shrink Sidebar (16 cols)" : "Expand Sidebar (24 cols)"
        focus: true
        onClicked: anchorsDemo.wideSidebar = !anchorsDemo.wideSidebar
    }
}
