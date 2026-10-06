pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Controls

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Space) {
            root.loaderActive = !root.loaderActive
            event.accepted = true
        } else if (event.key === Qt.Key_S) {
            root.showExecution = !root.showExecution
            event.accepted = true
        } else if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    Rectangle {
        id: root

        width: 80
        height: 24
        color: "#111827"
        focus: true

        property bool loaderActive: true
        property bool showExecution: false
        property int instancesCreated: 0

        readonly property string loaderState: contentLoader.status === Loader.Ready
            ? qsTr("Ready")
            : contentLoader.status === Loader.Loading
                ? qsTr("Loading")
                : contentLoader.status === Loader.Error
                    ? qsTr("Error")
                    : qsTr("Null")


        Column {
            anchors.fill: parent
            anchors.margins: 1
            spacing: 0

            Text {
                text: qsTr("QtUI Loader Demo")
                color: "cyan"
                bold: true
            }

            Text {
                text: qsTr("The loaded component is destroyed when inactive and recreated when enabled.")
                color: "gray"
            }

            Row {
                spacing: 2

                Button {
                    text: root.loaderActive ? qsTr("Unload [Space]") : qsTr("Load [Space]")
                    onClicked: root.loaderActive = !root.loaderActive
                }

                Button {
                    text: root.showExecution
                        ? qsTr("Show discovery [S]")
                        : qsTr("Show execution [S]")
                    onClicked: root.showExecution = !root.showExecution
                }
            }

            Rectangle {
                width: parent.width
                height: 12
                color: "#172033"
                border.style: "double"
                border.color: contentLoader.status === Loader.Ready ? "green" : "yellow"

                Loader {
                    id: contentLoader

                    anchors.fill: parent
                    anchors.margins: 2
                    active: root.loaderActive
                    sourceComponent: root.showExecution
                        ? executionComponent
                        : discoveryComponent

                    onLoaded: {
                        if (contentLoader.status !== Loader.Ready)
                            return
                        root.instancesCreated += 1
                        contentLoader.item.generation = root.instancesCreated
                    }
                }
            }

            Text {
                text: qsTr("Status: %1 | instances created: %2")
                    .arg(root.loaderState)
                    .arg(root.instancesCreated)
                color: contentLoader.status === Loader.Error ? "red" : "white"
            }

            Text {
                visible: contentLoader.status === Loader.Error
                text: contentLoader.errorString
                color: "red"
            }

            Text {
                text: qsTr("Press Space to load/unload, S to switch component, Q to quit.")
                color: "dimgray"
            }
        }

        Component {
            id: discoveryComponent

            Column {
                id: configurationInstance
                property int generation: 0

                spacing: 1

                Text {
                    text: qsTr("Adapter discovery")
                    color: "green"
                    bold: true
                }

                Text { text: qsTr("Executable: cargo") }
                Text { text: qsTr("Workspace: 4 packages, 7 targets") }

                CheckBox {
                    text: qsTr("Include optional features")
                    checked: true
                }

                Text {
                    text: qsTr("Loaded instance #%1").arg(configurationInstance.generation)
                    color: "gray"
                }
            }
        }

        Component {
            id: executionComponent

            Column {
                id: executionInstance
                property int generation: 0

                spacing: 1

                Text {
                    text: qsTr("Command execution")
                    color: "magenta"
                    bold: true
                }

                Text { text: qsTr("cargo build --workspace") }

                Switch {
                    text: qsTr("Release mode")
                }

                Text {
                    text: qsTr("Loaded instance #%1").arg(executionInstance.generation)
                    color: "gray"
                }
            }
        }
    }
}
