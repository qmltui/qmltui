pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// One QStandardItemModel, styled the Qt Widgets way, shown by three views:
// a ListView with StandardItemDelegate, and TableView and TreeView with
// their built-in painters. Toggling a check box updates all three.
Terminal {
    id: root

    ReleaseChecklistModel {
        id: checklist
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        }
    }

    Rectangle {
        width: 80
        height: 24

        Text {
            x: 1
            y: 0
            width: 78
            text: "STANDARD ITEM ROLES — ONE QStandardItemModel, THREE VIEWS"
            color: "cyan"
            bold: true
        }
        Text {
            x: 1
            y: 1
            width: 78
            text: "Up/Down select, Space toggles the check box in the model, q quits."
        }

        Rectangle {
            x: 1
            y: 3
            width: 36
            height: 9
            border.style: Border.Single
            border.color: "cyan"
            Text {
                x: 1
                y: 0
                text: " ListView + StandardItemDelegate "
                color: "cyan"
            }
            ListView {
                id: list
                objectName: "list"
                anchors.fill: parent
                focus: true
                model: checklist
                currentIndex: 0
                delegate: StandardItemDelegate {}
                // ListView turns Space and Return into itemActivated.
                onItemActivated: index => checklist.toggle(index)
            }
        }

        Rectangle {
            x: 37
            y: 3
            width: 42
            height: 9
            border.style: Border.Single
            border.color: "cyan"
            Text {
                x: 1
                y: 0
                text: " TableView, built-in painter "
                color: "cyan"
            }
            TableView {
                objectName: "table"
                anchors.fill: parent
                model: checklist
                // Titles and the right-aligned ETA come from the model's
                // headerData(); only the widths are set here.
                columnWidthProvider: column => column === 0 ? 22 : column === 1 ? 10 : 8
                selectedRow: list.currentIndex
            }
        }

        Rectangle {
            x: 1
            y: 13
            width: 78
            height: 8
            border.style: Border.Single
            border.color: "cyan"
            Text {
                x: 1
                y: 0
                text: " TreeView, built-in painter "
                color: "cyan"
            }
            TreeView {
                objectName: "tree"
                anchors.fill: parent
                model: checklist
                currentIndex: list.currentIndex
            }
        }

        Text {
            x: 1
            y: 22
            width: 78
            objectName: "status"
            text: checklist.doneCount + " of " + checklist.rowCount() + " tasks done"
            color: "yellow"
        }
    }
}
