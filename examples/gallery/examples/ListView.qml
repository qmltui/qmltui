pragma ComponentBehavior: Bound
import Qamui

// ListView: Delegates over a model. One tile of the Qamui gallery (examples/gallery)
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
        x: 2
        y: 1
        spacing: 1
        Text { text: "Up/Down move the current item; each row is a delegate over a JS array."; color: "#6e7b8b" }
        Rectangle {
            width: 50
            height: 8
            border.style: Border.Single
            border.color: "#f472b6"
            ListView {
                id: people
                anchors.fill: parent
                anchors.margins: 1
                focus: true
                clip: true
                model: [
                    { name: "Ada Lovelace", role: "Analyst", stars: 5 },
                    { name: "Grace Hopper", role: "Compiler engineer", stars: 5 },
                    { name: "Linus Torvalds", role: "Kernel", stars: 4 },
                    { name: "Margaret Hamilton", role: "Flight software", stars: 5 },
                    { name: "Dennis Ritchie", role: "Language design", stars: 4 },
                    { name: "Barbara Liskov", role: "Type theory", stars: 4 },
                    { name: "Ken Thompson", role: "Unix", stars: 4 },
                    { name: "Radia Perlman", role: "Networking", stars: 3 }
                ]
                delegate: Row {
                    id: person
                    required property int index
                    required property var modelData
                    width: people.width
                    spacing: 1
                    Text { text: person.ListView.isCurrentItem ? "›" : " "; color: "#f472b6" }
                    Text { width: 20; text: person.modelData.name; color: person.ListView.isCurrentItem ? "#f8fafc" : "#c9d1d9"; bold: person.ListView.isCurrentItem }
                    Text { width: 18; text: person.modelData.role; color: "#6e7b8b" }
                    Text { text: "★".repeat(person.modelData.stars); color: "#f9c74f" }
                }
            }
        }
        Text {
            readonly property var person: people.currentIndex >= 0 ? people.model[people.currentIndex] : undefined
            text: person ? "Current: " + person.name : "No current item"
            color: "#f472b6"
        }
    }
}
