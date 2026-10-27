pragma ComponentBehavior: Bound
import QmlTui

// Layouts: RowLayout, GridLayout, and Row positioners.
Column {
    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "RowLayout: Layout.preferredWidth and Layout.fillWidth"
        color: "#8b949e"
    }

    RowLayout {
        width: 48
        height: 3
        spacing: 1

        Rectangle {
            Layout.preferredWidth: 10
            Layout.fillHeight: true
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "10"
                color: "#79c0ff"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#122d24"

            Text {
                anchors.centerIn: parent
                text: "fillWidth"
                color: "#2cde85"
                bold: true
            }
        }

        Rectangle {
            Layout.preferredWidth: 10
            Layout.fillHeight: true
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "10"
                color: "#79c0ff"
            }
        }
    }

    Text {
        text: "GridLayout: 4 columns, one cell spanning two"
        color: "#8b949e"
    }

    GridLayout {
        id: grid
        width: 48
        height: 3
        columns: 4
        columnSpacing: 0
        rowSpacing: 1

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "0,0"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "0,1"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.columnSpan: 2
            Layout.preferredHeight: 1
            color: "#1f2937"

            Text {
                anchors.centerIn: parent
                text: "0,2 span 2"
                color: "#f2cc60"
                bold: true
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "1,0"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "1,1"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "1,2"
                color: "#c9d1d9"
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: "#161b22"

            Text {
                anchors.centerIn: parent
                text: "1,3"
                color: "#c9d1d9"
            }
        }
    }

    Text {
        text: "Row: children keep their explicit width"
        color: "#8b949e"
    }

    Row {
        spacing: 1

        Rectangle {
            width: 10
            height: 1
            color: "#122d24"

            Text {
                anchors.centerIn: parent
                text: "10"
                color: "#2cde85"
            }
        }

        Rectangle {
            width: 16
            height: 1
            color: "#00a651"

            Text {
                anchors.centerIn: parent
                text: "16"
                color: "#0d1117"
                bold: true
            }
        }

        Rectangle {
            width: 20
            height: 1
            color: "#2cde85"

            Text {
                anchors.centerIn: parent
                text: "20"
                color: "#0d1117"
                bold: true
            }
        }
    }
}
