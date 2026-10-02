import QtQuick
import Qtui

Terminal {
    id: root
    width: 80
    height: 24
    backgroundColor: "#1E1E1E"

    FocusScope {
        anchors.fill: parent
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

        // Main Layout Container
        FlexLayout {
            id: mainFlex
            x: 2
            y: 5
            width: 76
            height: 15
            flexDirection: FlexLayout.Row
            flexWrap: FlexLayout.NoWrap
            alignItems: FlexLayout.AlignStretch

            border.style: "ascii"
            border.color: "dimgray"

            // Box 1 (Grow 1)
            Text {
                text: "Box 1\n(Grow: 1)"
                flexGrow: 1.0
                backgroundColor: "blue"
                color: "white"
            }

            // Box 2 (Grow 2)
            Text {
                text: "Box 2\n(Grow: 2)"
                flexGrow: 2.0
                backgroundColor: "green"
                color: "black"
            }

            // Box 3 (Grow 1)
            Text {
                text: "Box 3\n(Grow: 1)"
                flexGrow: 1.0
                backgroundColor: "red"
                color: "white"
            }

            // Box 4 (Fixed width 12, no grow)
            Text {
                text: "Box 4\n(Width: 12)"
                flexBasis: 12
                backgroundColor: "purple"
                color: "white"
            }
        }

        Text {
            x: 2
            y: 22
            text: "Press [Q] or [Esc] to exit."
            color: "dimgray"
        }
    }
}
