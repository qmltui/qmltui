pragma ComponentBehavior: Bound
import QmlTui

// ListView: Virtualized vertical list instantiating a delegate per visible row.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "Up/Down move currentIndex; ListView.isCurrentItem marks the active delegate."
        color: "#8b949e"
    }

    Rectangle {
        width: 50
        height: 10
        color: "#161b22"

        ListView {
            id: people
            anchors {
                fill: parent
                margins: 1
            }
            focus: true
            clip: true
            currentIndex: 0
            model: [
                {
                    name: "Ada Lovelace",
                    role: "Analyst",
                    stars: 5
                },
                {
                    name: "Grace Hopper",
                    role: "Compilers",
                    stars: 5
                },
                {
                    name: "Linus Torvalds",
                    role: "Kernel",
                    stars: 4
                },
                {
                    name: "Margaret Hamilton",
                    role: "Flight SW",
                    stars: 5
                },
                {
                    name: "Dennis Ritchie",
                    role: "C & Unix",
                    stars: 4
                },
                {
                    name: "Barbara Liskov",
                    role: "Type theory",
                    stars: 4
                },
                {
                    name: "Ken Thompson",
                    role: "Unix",
                    stars: 4
                },
                {
                    name: "Radia Perlman",
                    role: "Networking",
                    stars: 3
                }
            ]

            delegate: Row {
                id: person
                required property int index
                required property var modelData

                width: people.width
                spacing: 1

                Text {
                    text: person.ListView.isCurrentItem ? "›" : " "
                    color: "#00ff66"
                }

                Text {
                    width: 18
                    text: person.modelData.name
                    color: person.ListView.isCurrentItem ? "#00ff66" : "#c9d1d9"
                    bold: person.ListView.isCurrentItem
                }

                Text {
                    width: 13
                    text: person.modelData.role
                    color: "#8b949e"
                }

                Text {
                    text: "★".repeat(person.modelData.stars)
                    color: "#f2cc60"
                }
            }
        }
    }

    Text {
        readonly property var person: people.currentIndex >= 0 ? people.model[people.currentIndex] : undefined
        text: person ? "Current: " + person.name : "No current item"
        color: "#2cde85"
    }
}
