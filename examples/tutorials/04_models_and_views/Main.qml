pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls
import QtQml.Models

Terminal {
    id: root
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }
    ListModel {
        id: services
        ListElement { service: "Gateway" }
        ListElement { service: "Worker" }
        ListElement { service: "Database" }
    }
    Column {
        x: 2; y: 1; spacing: 1
        Text { text: "Services" }
        ListView {
            id: list
            width: 30; height: 4
            focus: true
            currentIndex: 0
            model: services
            delegate: Text {
                required property string service
                text: service
                bold: ListView.isCurrentItem
            }
        }
        TableView {
            width: 50; height: 6
            showOuterBorder: true
            showColumnGrid: true
            columns: [{ title: "Service", width: 22 }, { title: "State", width: 20 }]
            model: [["Gateway", "Ready"], ["Worker", "Idle"]]
        }
    }
}
