pragma ComponentBehavior: Bound

import QtQml
import Qamui

Terminal {
    id: root
    color: "black"

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            listView.contentY = Math.min(listView.contentY + 1, Math.max(0, listView.contentHeight - listView.height))
            event.accepted = true
        } else if (event.key === Qt.Key_Up) {
            listView.contentY = Math.max(listView.contentY - 1, 0)
            event.accepted = true
        }
    }

    property var directoryModel: [
        { name: "Alice", role: "Engineer", stars: 5 },
        { name: "Bob", role: "Designer", stars: 3 },
        { name: "Charlie", role: "Manager", stars: 4 },
        { name: "Diana", role: "CEO", stars: 5 },
        { name: "Eve", role: "HR", stars: 2 },
        { name: "Frank", role: "Sales", stars: 4 },
        { name: "Grace", role: "Marketing", stars: 3 },
        { name: "Heidi", role: "Support", stars: 5 },
        { name: "Ivan", role: "IT", stars: 4 },
        { name: "Judy", role: "Developer", stars: 3 }
    ]

    Column {
        anchors.fill: parent
        anchors.margins: 2
        spacing: 1

        Text {
            text: "--- Employee Directory ---"
            color: "cyan"
        }

        Text {
            text: "Use Up/Down arrows to scroll. Press 'Q' to quit."
            color: "gray"
        }

        ListView {
            id: listView
            width: parent.width
            height: parent.height - 4
            clip: true
            spacing: 1
            model: root.directoryModel

            delegate: Rectangle {
                id: listDelegate

                required property int index
                required property var modelData

                width: listView.width
                height: 3
                border.style: Border.Single
                border.color: ListView.isCurrentItem ? "yellow" : "blue"

                Column {
                    x: 2
                    y: 0
                    Text {
                        text: "Name: " + listDelegate.modelData.name
                        color: "white"
                    }
                    Row {
                        spacing: 2
                        Text {
                            text: "Role: " + listDelegate.modelData.role
                            color: "gray"
                        }
                        Row {
                            spacing: 1
                            Text { text: "Rating: " }
                            Repeater {
                                model: listDelegate.modelData.stars
                                delegate: Text {
                                    text: "★"
                                    color: "yellow"
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
