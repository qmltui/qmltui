pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls
import Qamui.Image

// Image: pixels as cells. One tile of the Qamui gallery (examples/apps/gallery)
// and a complete app on its own. The picture ships inside the binary as a
// resource, so the same file works in a terminal and in the browser.
Terminal {
    id: root
    color: "#0b0f14"

    // Braille first: eight dots per cell keep the picture's shape at any size,
    // and in xterm.js (the gallery page) it reads far better than half-block,
    // whose two colour bands per cell smear on a browser font.
    readonly property var modes: [
        { mode: Image.Braille, name: "Braille", blurb: "eight dots per cell, one colour" },
        { mode: Image.HalfBlock, name: "HalfBlock", blurb: "two pixels per cell, true colour" },
        { mode: Image.ASCII, name: "ASCII", blurb: "brightness as characters" },
        { mode: Image.Auto, name: "Auto", blurb: "kitty or sixel where the terminal has them, else half-block" }
    ]
    property int modeIndex: 0

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Column {
        x: 2
        y: 1
        spacing: 1
        width: Math.min(60, root.width - 4)
        Text { text: "M cycles the render mode. Terminal cells stand in for pixels."; color: "#6e7b8b" }
        // M is handled on the panel, which takes focus, so it also works when
        // this file is hosted inside the gallery.
        Panel {
            width: parent.width
            height: 14
            title: "Image  ·  " + root.modes[root.modeIndex].name
            focusable: true
            focus: true
            Keys.onPressed: event => {
                if (event.key === Qt.Key_M) {
                    root.modeIndex = (root.modeIndex + 1) % root.modes.length
                    event.accepted = true
                }
            }
            Image {
                id: picture
                anchors.fill: parent
                anchors.margins: 1
                anchors.topMargin: 2
                source: "qrc:/gallery/assets/panda.png"
                fillMode: Image.PreserveAspectFit
                renderMode: root.modes[root.modeIndex].mode
            }
        }
        Text {
            text: root.modes[root.modeIndex].name + ": " + root.modes[root.modeIndex].blurb
            color: "#7ee0ff"
        }
        Text {
            text: picture.status === Image.Ready
                  ? "Source " + picture.sourceSize.width + "x" + picture.sourceSize.height + " px"
                  : picture.status === Image.Error ? "Image failed to load" : "Loading"
            color: "#65e6a0"
            bold: true
        }
    }
}
