pragma ComponentBehavior: Bound
import QtQml
import Qtui
import Qtui.Controls
import Qtui.Image
// FrameSource is a C++ type of this QML module; no import needed for own types.

// Two ways to show pixels the application produces itself, side by side.
// Left: a C++ object owns a QImage and the Image binds to it (Image.image).
// Right: the Image asks a qtui::ImageProvider by URL; the tick in the query
// requests a new picture each time it changes (Image.source = image://...).
Terminal {
    id: root

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
        } else if (event.key === Qt.Key_Space) {
            feed.running = !feed.running
        } else {
            return
        }
        event.accepted = true
    }

    // Shape 1: the picture lives in C++ and is pushed to QML as a property.
    FrameSource { id: feed; interval: 100 }

    Column {
        x: 2
        y: 1
        spacing: 1
        Text { text: "Live Image: pictures from memory"; color: "#7ee0ff"; bold: true }
        Text { text: "Space pauses, Q quits.  Tick " + feed.tick + (feed.running ? "" : "  (paused)"); color: "#6e7b8b" }
        Row {
            spacing: 2
            Panel {
                width: 36
                height: 14
                title: "image: feed.frame  (Braille)"
                Image {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.topMargin: 2
                    image: feed.frame                 // repaints on every frameChanged()
                    fillMode: Image.Stretch
                    renderMode: Image.Braille
                }
            }
            Panel {
                width: 36
                height: 14
                title: "source: image://frames/radar  (HalfBlock)"
                Image {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.topMargin: 2
                    // A new URL per tick asks the provider again; it renders
                    // to this item's pixel grid.
                    source: "image://frames/radar?tick=" + feed.tick
                    fillMode: Image.PreserveAspectFit
                    renderMode: Image.HalfBlock
                }
            }
        }
        Text {
            width: 74
            text: "Left: FrameSource (C++) redraws a QImage on a timer; QML binds Image.image to it.\n"
                + "Right: FrameProvider (C++) is registered as \"frames\"; the query carries the tick."
            color: "#6e7b8b"
        }
    }
}
