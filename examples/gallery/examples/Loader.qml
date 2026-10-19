pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// Loader: Dynamically creates, swaps, and unloads QML components on demand.
Column {
    id: loaderDemo

    property bool details: false
    property int created: 0

    spacing: 1

    Text {
        text: "Loader swaps sourceComponent or unloads when active is false."
        color: "#8b949e"
    }

    Row {
        spacing: 2

        Button {
            text: loaderDemo.details ? "Show summary" : "Show details"
            focus: true
            onClicked: loaderDemo.details = !loaderDemo.details
        }

        Button {
            text: pageLoader.active ? "Unload" : "Load"
            onClicked: pageLoader.active = !pageLoader.active
        }
    }

    Rectangle {
        width: Math.min(44, parent.width)
        height: 5
        border {
            style: pageLoader.status === Loader.Ready ? Border.Single : Border.Ascii
            color: pageLoader.status === Loader.Ready ? "#fcd34d" : "#8b949e"
        }

        Loader {
            id: pageLoader
            anchors {
                fill: parent
                margins: 1
            }
            sourceComponent: loaderDemo.details ? detailsComponent : summaryComponent
            onLoaded: loaderDemo.created++
        }
    }

    Text {
        text: "status " + (pageLoader.status === Loader.Ready ? "Ready" : pageLoader.status === Loader.Null ? "Null" : "Loading") + ", items created: " + loaderDemo.created
        color: "#fcd34d"
    }

    Component {
        id: summaryComponent

        Column {
            Text {
                text: "Summary"
                bold: true
                color: "#fcd34d"
            }

            Text {
                text: "3 services, all healthy"
                color: "#c9d1d9"
            }
        }
    }

    Component {
        id: detailsComponent

        Column {
            Text {
                text: "Details"
                bold: true
                color: "#fcd34d"
            }

            Text {
                text: "gateway ✓   auth ✓   billing ✓"
                color: "#c9d1d9"
            }

            Text {
                text: "last check 4 s ago"
                color: "#8b949e"
            }
        }
    }
}
