pragma ComponentBehavior: Bound
import Qtui
import Qtui.Controls

// States: State + Transition. One tile of the Qtui gallery (examples/apps/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Column {
        id: statesDemo
        x: 2
        y: 1
        property bool active: false
        spacing: 1
        Text { text: "PropertyChanges describe each state; the Transition animates the colour between them."; color: "#6e7b8b" }
        Button { text: statesDemo.active ? "Switch off" : "Switch on"; focus: true; onClicked: statesDemo.active = !statesDemo.active }
        Rectangle {
            id: card
            width: 44
            height: 3
            color: "#331111"
            border.style: "single"
            border.color: "#7f1d1d"
            Text { id: cardLabel; anchors.centerIn: parent; text: "● OFF"; bold: true; color: "#fca5a5" }
            states: [
                State {
                    name: "on"
                    when: statesDemo.active
                    PropertyChanges { target: card; properties: { "color": "#113311", "border.color": "#166534" } }
                    PropertyChanges { target: cardLabel; properties: { "text": "● ON", "color": "#86efac" } }
                }
            ]
            transitions: [ Transition { ColorAnimation { duration: 400 } } ]
        }
        Text { text: "state: " + (card.state === "" ? "(base)" : card.state); color: "#34d399" }
    }
}
