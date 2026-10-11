import QtQml
import Qamui


Terminal {

    ProcessTableModel {
        id: processModel
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        // Live Linux top command header statistics bound to C++ ProcessTableModel
        Text {
            text: processModel.uptimeText
            color: "white"
        }
        Text {
            text: processModel.tasksText
            color: "white"
        }
        Text {
            text: processModel.cpuText
            color: "cyan"
        }
        Text {
            text: processModel.memText
            color: "yellow"
        }
        Text {
            text: processModel.swapText
            color: "yellow"
        }

        Text { text: ""; height: 1 } // Blank separator

        // Process TableView dynamically filling remaining terminal height
        TableView {
            focus: true
            width: parent.width
            height: Math.max(5, parent.height - 8)
            headerColor: "black"
            headerBackgroundColor: "white"
            selectionColor: "white"
            selectionBackgroundColor: "blue"
            alternatingRows: true
            showColumnGrid: true

            columns: processModel.columnDefinitions()
            model: processModel

            // Custom JS Cell Formatter to colorize %CPU and %MEM
            cellFormat: (row, col, value) => {
                if (col === 7 && parseFloat(value) > 1.0) {
                    return { text: value, color: "red", bold: true }
                }
                if (col === 1) {
                    return { text: value, color: "cyan" }
                }
                return { text: value }
            }

            onRowActivated: (idx) => {
                statusText.text = "Selected process row #" + idx
            }
        }

        Text {
            id: statusText
            text: "Press [Q] or [ESC] to quit top monitor"
            color: "gray"
        }
    }
}
