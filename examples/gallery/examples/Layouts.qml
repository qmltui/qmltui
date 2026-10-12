pragma ComponentBehavior: Bound
import QmlTui

// Layouts: Row, Column, GridLayout. One tile of the QmlTui gallery (examples/gallery)
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
        width: 66
        spacing: 1
        Text { text: "RowLayout: children size through Layout.preferredWidth and Layout.fillWidth"; color: "#6e7b8b" }
        RowLayout {
            width: parent.width
            height: 2
            spacing: 1
            Rectangle { Layout.preferredWidth: 12; Layout.fillHeight: true; color: "#7c2d12"; Text { anchors.centerIn: parent; text: "12"; color: "#c9d1d9" } }
            Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; color: "#a16207"; Text { anchors.centerIn: parent; text: "fillWidth"; color: "#c9d1d9" } }
            Rectangle { Layout.preferredWidth: 12; Layout.fillHeight: true; color: "#7c2d12"; Text { anchors.centerIn: parent; text: "12"; color: "#c9d1d9" } }
        }
        Text { text: "GridLayout: 4 columns, one cell spanning two"; color: "#6e7b8b" }
        GridLayout {
            width: parent.width
            height: 2
            columns: 4
            columnSpacing: 1
            rowSpacing: 0
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e3a5f"; Text { anchors.centerIn: parent; text: "0,0"; color: "#c9d1d9" } }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e3a5f"; Text { anchors.centerIn: parent; text: "0,1"; color: "#c9d1d9" } }
            Rectangle { Layout.fillWidth: true; Layout.columnSpan: 2; Layout.preferredHeight: 1; color: "#3b0764"; Text { anchors.centerIn: parent; text: "0,2 span 2"; color: "#c9d1d9" } }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e3a5f"; Text { anchors.centerIn: parent; text: "1,0"; color: "#c9d1d9" } }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e3a5f"; Text { anchors.centerIn: parent; text: "1,1"; color: "#c9d1d9" } }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e3a5f"; Text { anchors.centerIn: parent; text: "1,2"; color: "#c9d1d9" } }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: "#1e3a5f"; Text { anchors.centerIn: parent; text: "1,3"; color: "#c9d1d9" } }
        }
        Text { text: "Row: children keep their own width"; color: "#6e7b8b" }
        Row {
            spacing: 2
            Rectangle { width: 10; height: 1; color: "#164e63"; Text { anchors.centerIn: parent; text: "10"; color: "#c9d1d9" } }
            Rectangle { width: 20; height: 1; color: "#155e75"; Text { anchors.centerIn: parent; text: "20"; color: "#c9d1d9" } }
            Rectangle { width: 30; height: 1; color: "#0e7490"; Text { anchors.centerIn: parent; text: "30"; color: "#c9d1d9" } }
        }
    }
}
