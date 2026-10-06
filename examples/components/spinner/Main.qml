import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Panel {
        anchors.fill: parent
        anchors.margins: 1
        title: "Activity Indicators (Spinner Controls)"
        titleAlignment: Qt.AlignHCenter
        titleColor: "yellow"
        border.color: "cyan"

        Column {
            anchors.fill: parent
            anchors.topMargin: 1
            anchors.leftMargin: 2
            
            spacing: 1
            Row {
                spacing: 2
                Spinner {
                    style: Spinner.Style.Dots
                    color: "cyan"
                }
                Text { text: "Loading network requests (Dots)"; color: "cyan" }
            }
            Row {
                spacing: 2
                Spinner {
                    style: Spinner.Style.Line
                    color: "green"
                }
                Text { text: "Loading system resources (Line)"; color: "green" }
            }

            Row {
                spacing: 2
                Spinner {
                    style: Spinner.Style.Pipe
                    color: "yellow"
                }
                Text { text: "Encrypting sensitive data (Pipe)"; color: "yellow" }
            
            }

            Row {
                spacing: 2
                Spinner {
                    style: Spinner.Style.Block
                    color: "magenta"
                }
                Text { text: "Initializing database connection (Block)"; color: "magenta" }
            }

            Row {
                spacing: 2
                Spinner {
                    style: Spinner.Style.Bounce
                    color: "lightgreen"
                }
                Text { text: "Connecting to remote server (Bounce)"; color: "lightgreen" }
             
            }
        }
    }
}
