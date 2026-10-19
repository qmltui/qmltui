pragma ComponentBehavior: Bound
import QmlTui

// ScrollView: Scrollable viewport container with automatic scrollbar rendering.
Column {
    spacing: 1

    Text {
        text: "Up/Down, PageUp/PageDown, or mouse wheel scroll the viewport."
        color: "#8b949e"
    }

    Rectangle {
        width: Math.min(46, parent.width)
        height: 9
        border {
            style: Border.Single
            color: "#67e8f9"
        }

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
                        color: index % 10 === 9 ? "#67e8f9" : "#c9d1d9"
                    }
                }
            }
        }
    }

    Text {
        text: "contentY " + scroller.contentY + " of " + scroller.contentHeight
        color: "#67e8f9"
    }
}
