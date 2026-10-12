pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls

// Loader: Create, swap on demand. One tile of the Qamui gallery (examples/gallery)
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
        id: loaderDemo
        x: 2
        y: 1
        property bool details: false
        property int created: 0
        spacing: 1
        Text { text: "The Loader destroys the old item and creates the new one each time sourceComponent changes."; color: "#6e7b8b" }
        Row {
            spacing: 2
            Button { text: loaderDemo.details ? "Show summary" : "Show details"; focus: true; onClicked: loaderDemo.details = !loaderDemo.details }
            Button { text: pageLoader2.active ? "Unload" : "Load"; onClicked: pageLoader2.active = !pageLoader2.active }
        }
        Rectangle {
            width: 50
            height: 5
            border.style: pageLoader2.status === Loader.Ready ? Border.Single : Border.Ascii
            border.color: pageLoader2.status === Loader.Ready ? "#fcd34d" : "#6e7b8b"
            Loader {
                id: pageLoader2
                anchors.fill: parent
                anchors.margins: 1
                sourceComponent: loaderDemo.details ? detailsComponent : summaryComponent
                onLoaded: loaderDemo.created++
            }
        }
        Text { text: "status " + (pageLoader2.status === Loader.Ready ? "Ready" : pageLoader2.status === Loader.Null ? "Null" : "Loading") + ", items created so far: " + loaderDemo.created; color: "#fcd34d" }
        Component {
            id: summaryComponent
            Column {
                Text { text: "Summary"; bold: true; color: "#fcd34d" }
                Text { text: "3 services, all healthy"; color: "#c9d1d9" }
            }
        }
        Component {
            id: detailsComponent
            Column {
                Text { text: "Details"; bold: true; color: "#fcd34d" }
                Text { text: "gateway ✓   auth ✓   billing ✓"; color: "#c9d1d9" }
                Text { text: "last check 4 s ago"; color: "#6e7b8b" }
            }
        }
    }
}
