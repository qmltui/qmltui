pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Controls
import Qtui.Image

// A dynamic picture served by a qtui::AsyncImageProvider on a dedicated thread.
// The Image asks the provider by URL; the tick in the query requests a new sweep
// each time it changes (Image.source = image://frames/radar?tick=...).
// Draws at the Image's renderSize, so the sweep is crisp whether the terminal
// shows true pixels (kitty, sixel) or half-block cells.
Terminal {
    id: root

    property int tick: 0
    property bool running: true

    Timer {
        interval: 100
        running: root.running
        repeat: true
        onTriggered: root.tick++
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
        } else if (event.key === Qt.Key_Space) {
            root.running = !root.running
        } else {
            return
        }
        event.accepted = true
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        anchors.topMargin: 1
        anchors.bottomMargin: 1
        spacing: 1

        Text { text: "Live Image: pictures from memory"; color: "#7ee0ff"; bold: true }
        Text {
            text: "Space pauses, Q quits.  Tick " + root.tick + (root.running ? "" : "  (paused)")
                + "   frames " + radar.renderSize.width + "x" + radar.renderSize.height + " px"
            color: "#6e7b8b"
        }

        Panel {
            width: parent.width
            height: Math.max(6, parent.height - 7)
            title: "source: image://frames/radar"

            Image {
                id: radar
                anchors.fill: parent
                anchors.margins: 1
                anchors.topMargin: 2
                // A new URL per tick asks the provider again; it receives
                // this item's renderSize and renders to it.
                source: "image://frames/radar?tick=" + root.tick
                retainWhileLoading: true
                fillMode: Image.Stretch
                renderMode: Image.Auto
            }
        }

        Text {
            text: "RadarProvider (C++) registered as \"frames\"; the query carries the tick, the request the size."
            color: "#6e7b8b"
        }
    }
}
