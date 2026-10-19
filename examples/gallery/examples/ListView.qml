pragma ComponentBehavior: Bound
import QmlTui

// ListView: Virtualized vertical list instantiating a delegate per visible row.
Column {
    spacing: 1

    Text {
        text: "Up/Down move currentIndex; ListView.isCurrentItem marks the active delegate."
        color: "#8b949e"
    }

    Rectangle {
        width: Math.min(48, parent.width)
        height: 8
        border {
            style: Border.Single
            color: "#f472b6"
        }

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
                    color: "#f472b6"
                }

                Text {
                    width: 18
                    text: person.modelData.name
                    color: person.ListView.isCurrentItem ? "#f8fafc" : "#c9d1d9"
                    bold: person.ListView.isCurrentItem
                }

                Text {
                    width: 13
                    text: person.modelData.role
                    color: "#8b949e"
                }

                Text {
                    text: "★".repeat(person.modelData.stars)
                    color: "#f9c74f"
                }
            }
        }
    }

    Text {
        readonly property var person: people.currentIndex >= 0 ? people.model[people.currentIndex] : undefined
        text: person ? "Current: " + person.name : "No current item"
        color: "#f472b6"
    }
}
