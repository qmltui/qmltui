import QtQml 2.15
import Tui 1.0

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    id: rootWindow
    width: 80
    height: 24
    backgroundColor: "black"
    useAlternateScreen: true

    Rectangle {
        anchors.fill: parent
        color: "#111111"
        border.style: "double"
        border.color: "gray"

        // Header - anchored to top, left, and right of the parent
        Rectangle {
            id: header
            height: 3
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 1
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            color: "#222222"
            
            Text {
                anchors.fill: parent
                color: "yellow"
                bold: true
                text: "Robust Anchoring System Demo"
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Left Panel - anchored top to header's bottom, left to parent, bottom to parent
        Rectangle {
            id: leftPanel
            width: 25
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.topMargin: 1
            anchors.bottomMargin: 2
            color: "#333333"

            Text {
                anchors.fill: parent
                color: "cyan"
                text: " Left Panel\n [Sibling A]\n\n width: 25\n left: parent.left\n top: header.bottom"
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Content Area - anchored top to header's bottom, left to leftPanel's right, right and bottom to parent
        Rectangle {
            id: contentArea
            anchors.left: leftPanel.right
            anchors.right: parent.right
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.leftMargin: 2
            anchors.rightMargin: 2
            anchors.topMargin: 1
            anchors.bottomMargin: 2
            color: "#1d2b53"

            Text {
                anchors.fill: parent
                color: "white"
                text: "Content Area [Sibling B]\n\nanchors.left: leftPanel.right\nanchors.right: parent.right\nanchors.top: header.bottom\nanchors.bottom: parent.bottom\n\nPress 'q' to exit."
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }
    }
}
