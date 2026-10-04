import QtQuick
import Qtui
import Qtui.Controls
import Qtui.Image

Terminal {
    id: root
    fillTerminal: true
    useAlternateScreen: true
    color: "#0f172a"

    property string imageSource: (typeof userImagePath !== "undefined" && userImagePath !== "") ? userImagePath : "panda.png"
    property string statusMessage: "Loaded: " + imageSource

    // Explicit key handling for quit and quick preset switching
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key === Qt.Key_1) {
            pathInput.text = "panda.png";
            root.updateImageSource("panda.png");
            event.accepted = true;
        } else if (event.key === Qt.Key_2) {
            pathInput.text = "demo_image.png";
            root.updateImageSource("demo_image.png");
            event.accepted = true;
        } else if (event.key === Qt.Key_3) {
            pathInput.text = "map.png";
            root.updateImageSource("map.png");
            event.accepted = true;
        } else if (event.key === Qt.Key_4) {
            pathInput.text = "image2.jpg";
            root.updateImageSource("image2.jpg");
            event.accepted = true;
        } else if (event.key === Qt.Key_5) {
            pathInput.text = "qt_logo_icon_248726.webp";
            root.updateImageSource("qt_logo_icon_248726.webp");
            event.accepted = true;
        }
    }

    function updateImageSource(newPath) {
        root.imageSource = newPath;
        root.statusMessage = "Loaded: " + newPath;
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        // Title Header
        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: "single"
            border.color: "cyan"

            Text {
                anchors.fill: parent
                anchors.margins: 1
                text: "Qtui.Image Viewer Demo (Press Q or ESC to Quit)"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Subtitle / Instructions
        Text {
            width: parent.width
            text: "Type/paste image path & press Enter. Presets: [1] panda  [2] demo  [3] map  [4] photo"
            color: "#94a3b8"
        }

        // File Path Input Bar
        Row {
            spacing: 2

            Text {
                width: 14
                text: "Image Path:"
                color: pathInput.activeFocus ? "#38bdf8" : "#94a3b8"
                bold: pathInput.activeFocus
            }

            Rectangle {
                width: parent.width - 18
                height: 3
                color: pathInput.activeFocus ? "#1e293b" : "#0f172a"
                border.style: pathInput.activeFocus ? "double" : "single"
                border.color: pathInput.activeFocus ? "#38bdf8" : "#475569"

                TextInput {
                    id: pathInput
                    anchors.fill: parent
                    anchors.margins: 1
                    text: root.imageSource
                    color: activeFocus ? "#ffffff" : "#cbd5e1"
                    focusable: true

                    onAccepted: {
                        root.updateImageSource(text);
                    }
                }
            }
        }

        // 3-Panel Render Mode View
        Row {
            width: parent.width
            height: parent.height - 12
            spacing: 1

            Panel {
                title: "HalfBlock Mode"
                width: (parent.width - 2) / 3
                height: parent.height

                Image {
                    anchors.fill: parent
                    anchors.topMargin: 2
                    anchors.leftMargin: 1
                    anchors.rightMargin: 1
                    anchors.bottomMargin: 1
                    source: root.imageSource
                    fillMode: Image.PreserveAspectFit
                    renderMode: Image.HalfBlock
                }
            }

            Panel {
                title: "Braille Mode"
                width: (parent.width - 2) / 3
                height: parent.height

                Image {
                    anchors.fill: parent
                    anchors.topMargin: 2
                    anchors.leftMargin: 1
                    anchors.rightMargin: 1
                    anchors.bottomMargin: 1
                    source: root.imageSource
                    fillMode: Image.PreserveAspectFit
                    renderMode: Image.Braille
                }
            }

            Panel {
                title: "ASCII Mode"
                width: (parent.width - 2) / 3
                height: parent.height

                Image {
                    anchors.fill: parent
                    anchors.topMargin: 2
                    anchors.leftMargin: 1
                    anchors.rightMargin: 1
                    anchors.bottomMargin: 1
                    source: root.imageSource
                    fillMode: Image.PreserveAspectFit
                    renderMode: Image.ASCII
                }
            }
        }

        // Status Bar
        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: "single"
            border.color: "yellow"

            Text {
                anchors.fill: parent
                anchors.margins: 1
                text: "STATUS: " + root.statusMessage
                color: "yellow"
                bold: true
                verticalAlignment: Qt.AlignVCenter
            }
        }
    }
}
