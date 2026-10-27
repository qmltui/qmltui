pragma ComponentBehavior: Bound
import QmlTui

// ScrollView: Scrollable viewport container with automatic scrollbar rendering.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "Up/Down, PageUp/PageDown, or mouse wheel scroll the viewport."
        color: "#8b949e"
    }

    Rectangle {
        width: 48
        height: 10
        color: "#161b22"

        ScrollView {
            id: scroller
            anchors {
                fill: parent
                margins: 1
            }
            focus: true
            contentHeight: contentCol.implicitHeight

            Column {
                id: contentCol
                height: implicitHeight

                Repeater {
                    model: 40
                    delegate: Text {
                        required property int index
                        text: "line " + (index + 1) + (index % 10 === 9 ? "  ── section " + Math.floor(index / 10 + 1) : "")
                        color: index % 10 === 9 ? "#00ff66" : "#c9d1d9"
                    }
                }
            }
        }
    }

    Text {
        text: "contentY " + scroller.contentY + " of " + scroller.contentHeight
        color: "#2cde85"
    }
}
