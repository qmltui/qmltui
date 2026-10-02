import QtQuick
import Tui

Terminal {
    id: root
    width: 80
    height: 24
    backgroundColor: "#1E1E1E"
    focus: true

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_D) {
            if (mainFlex.flexDirection === FlexLayout.Row) {
                mainFlex.flexDirection = FlexLayout.Column
                infoText.text = "Controls: Press [D] toggle Direction (Column), [W] toggle Wrap (" + (mainFlex.flexWrap === FlexLayout.Wrap ? "Wrap" : "NoWrap") + ")"
            } else {
                mainFlex.flexDirection = FlexLayout.Row
                infoText.text = "Controls: Press [D] toggle Direction (Row), [W] toggle Wrap (" + (mainFlex.flexWrap === FlexLayout.Wrap ? "Wrap" : "NoWrap") + ")"
            }
            event.accepted = true
        } else if (event.key === Qt.Key_W) {
            if (mainFlex.flexWrap === FlexLayout.Wrap) {
                mainFlex.flexWrap = FlexLayout.NoWrap
                infoText.text = "Controls: Press [D] toggle Direction (" + (mainFlex.flexDirection === FlexLayout.Row ? "Row" : "Column") + "), [W] toggle Wrap (NoWrap)"
            } else {
                mainFlex.flexWrap = FlexLayout.Wrap
                infoText.text = "Controls: Press [D] toggle Direction (" + (mainFlex.flexDirection === FlexLayout.Row ? "Row" : "Column") + "), [W] toggle Wrap (Wrap)"
            }
            event.accepted = true
        }
    }

    // Header info
    Text {
        x: 2
        y: 1
        text: "<font color='orange'><b>Qtui FlexLayout Interactive Demo</b></font>"
        textFormat: Text.RichText
    }

    Text {
        id: infoText
        x: 2
        y: 3
        text: "Controls: Press [D] toggle Direction (Row), [W] toggle Wrap (NoWrap)"
        color: "gray"
    }

    // Main Layout Container wrapped in a Rectangle for borders
    Rectangle {
        x: 2
        y: 5
        width: 76
        height: 15
        border.style: "ascii"
        border.color: "dimgray"
        padding: 1

        FlexLayout {
            id: mainFlex
            anchors.fill: parent
            flexDirection: FlexLayout.Row
            flexWrap: FlexLayout.NoWrap
            alignItems: FlexLayout.AlignStretch

            // Box 1 (Grow 1)
            Rectangle {
                flexGrow: 1.0
                color: "blue"
                Text {
                    anchors.fill: parent
                    text: "Box 1\n(Grow: 1)"
                    color: "white"
                }
            }

            // Box 2 (Grow 2)
            Rectangle {
                flexGrow: 2.0
                color: "green"
                Text {
                    anchors.fill: parent
                    text: "Box 2\n(Grow: 2)"
                    color: "black"
                }
            }

            // Box 3 (Grow 1)
            Rectangle {
                flexGrow: 1.0
                color: "red"
                Text {
                    anchors.fill: parent
                    text: "Box 3\n(Grow: 1)"
                    color: "white"
                }
            }

            // Box 4 (Fixed width 12, no grow)
            Rectangle {
                flexBasis: 12
                color: "purple"
                Text {
                    anchors.fill: parent
                    text: "Box 4\n(Width: 12)"
                    color: "white"
                }
            }
        }
    }

    Text {
        x: 2
        y: 22
        text: "Press [Q] or [Esc] to exit."
        color: "dimgray"
    }
}
