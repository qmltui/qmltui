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
// Both draw at the Image's renderSize, so the frames are crisp whether the
// terminal shows true pixels (kitty, sixel) or half-block cells.
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
    // frameSize follows the Image, so a resize or a protocol change redraws
    // at the new resolution.
    FrameSource { id: feed; interval: 100; frameSize: scope.renderSize }

    Column {
        x: 2
        y: 1
        spacing: 1
        Text { text: "Live Image: pictures from memory"; color: "#7ee0ff"; bold: true }
        Text {
            text: "Space pauses, Q quits.  Tick " + feed.tick + (feed.running ? "" : "  (paused)")
                + "   frames " + scope.renderSize.width + "x" + scope.renderSize.height + " px"
            color: "#6e7b8b"
        }
        Row {
            spacing: 2
            Panel {
                width: 36
                height: 14
                title: "image: feed.frame"
                Image {
                    id: scope
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.topMargin: 2
                    image: feed.frame                 // repaints on every frameChanged()
                    fillMode: Image.Stretch           // the frame already has this exact size
                    renderMode: Image.Auto            // kitty or sixel where available, else half-block
                }
            }
            Panel {
                width: 36
                height: 14
                title: "source: image://frames/radar"
                Image {
                    anchors.fill: parent
                    anchors.margins: 1
                    anchors.topMargin: 2
                    // A new URL per tick asks the provider again; it receives
                    // this item's renderSize and renders to it.
                    source: "image://frames/radar?tick=" + feed.tick
                    fillMode: Image.Stretch
                    renderMode: Image.Auto
                }
            }
        }
        Text {
            width: 74
            text: "Left: FrameSource (C++) redraws a QImage on a timer at scope.renderSize; QML binds Image.image.\n"
                + "Right: FrameProvider (C++) registered as \"frames\"; the query carries the tick, the request the size."
            color: "#6e7b8b"
        }
    }
}
