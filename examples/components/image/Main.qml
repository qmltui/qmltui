import QtQml
import Qtui
import Qtui.Controls
import Qtui.Image

// Dedicated showcase for Qtui.Image rendering. The large image uses the Auto
// render mode: on a terminal that supports the kitty graphics protocol it is
// drawn with real pixels, and on every other terminal it falls back to the
// half-block glyph rendering. The status bar reports which path is active so
// the difference is legible even in a recording.
Terminal {
    id: root
    color: "#0f172a"

    property string userImagePath: "panda.png"
    property string imageSource: userImagePath.length > 0 ? userImagePath : "panda.png"

    // Render modes to cycle through with [M] for a side-by-side feel over time.
    readonly property var modes: [
        {
            mode: Image.Auto,
            name: "Auto"
        },
        {
            mode: Image.HalfBlock,
            name: "HalfBlock"
        },
        {
            mode: Image.Braille,
            name: "Braille"
        },
        {
            mode: Image.ASCII,
            name: "ASCII"
        },
        {
            mode: Image.Sixel,
            name: "Sixel"
        },
        {
            mode: Image.Kitty,
            name: "Kitty"
        }
    ]
    property int modeIndex: 0
    readonly property string modeName: modes[modeIndex].name

    readonly property bool kitty: capabilities.kittyGraphics
    readonly property bool sixel: capabilities.sixelGraphics
    // What Image will actually do for the current mode. Auto prefers kitty,
    // then sixel, then the half-block fallback.
    readonly property string activePath: (kitty && (modeName === "Auto" || modeName === "Kitty")) ? "kitty (real pixels)" : (sixel && (modeName === "Auto" || modeName === "Sixel")) ? "sixel (real pixels)" : "half-block fallback"

    function setImage(path) {
        root.userImagePath = path;
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key === Qt.Key_M) {
            root.modeIndex = (root.modeIndex + 1) % root.modes.length;
            event.accepted = true;
        } else if (event.key === Qt.Key_1) {
            root.setImage("panda.png");
            event.accepted = true;
        } else if (event.key === Qt.Key_2) {
            root.setImage("demo_image.png");
            event.accepted = true;
        } else if (event.key === Qt.Key_3) {
            root.setImage("map.png");
            event.accepted = true;
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 1

        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: "single"
            border.color: "cyan"

            Text {
                anchors.fill: parent
                anchors.margins: 1
                text: "Qtui.Image  —  Auto uses kitty graphics where available"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        Text {
            width: parent.width
            text: "[1-3] image   [M] render mode   [Q] quit"
            color: "#94a3b8"
        }

        Panel {
            title: "Image  (" + root.modeName + ")"
            width: parent.width
            height: parent.height - 10

            Image {
                anchors.fill: parent
                anchors.topMargin: 2
                anchors.leftMargin: 1
                anchors.rightMargin: 1
                anchors.bottomMargin: 1
                source: root.imageSource
                fillMode: Image.PreserveAspectFit
                renderMode: root.modes[root.modeIndex].mode
            }
        }

        Rectangle {
            width: parent.width
            height: 3
            color: "#1e293b"
            border.style: "single"
            border.color: "yellow"

            Text {
                anchors.fill: parent
                anchors.margins: 1
                // Image and mode first so they are never truncated on a narrow
                // terminal; the path and cell size follow.
                text: "Image: " + root.imageSource + "   Mode: " + root.modeName + "   Path: " + root.activePath + "   Cell: " + root.capabilities.cellPixelSize.width + "x" + root.capabilities.cellPixelSize.height
                color: "yellow"
                bold: true
                verticalAlignment: Qt.AlignVCenter
            }
        }
    }
}
