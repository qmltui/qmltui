pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Image

// Image: Renders bitmaps into terminal cells via Braille, HalfBlock, ASCII,
// or native terminal graphics protocols (Kitty / Sixel in Image.Auto mode).
ColumnLayout {
    id: imageDemo

    readonly property var modes: [
        {
            mode: Image.Braille,
            name: "Braille",
            blurb: "eight dots per cell, one colour"
        },
        {
            mode: Image.HalfBlock,
            name: "HalfBlock",
            blurb: "two pixels per cell, true colour"
        },
        {
            mode: Image.ASCII,
            name: "ASCII",
            blurb: "brightness as characters"
        },
        {
            mode: Image.Auto,
            name: "Auto",
            blurb: "kitty or sixel where supported, else half-block"
        }
    ]
    property int modeIndex: 0

    anchors.fill: parent
    spacing: 1
    focusPolicy: Qt.StrongFocus
    focus: true

    Keys.onPressed: event => {
        if (event.key === Qt.Key_M) {
            imageDemo.modeIndex = (imageDemo.modeIndex + 1) % imageDemo.modes.length;
            event.accepted = true;
        }
    }

    Text {
        text: "Press M to cycle renderMode."
        color: "#8b949e"
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true
        color: "#161b22"

        Image {
            id: picture
            anchors {
                fill: parent
                margins: 1
            }
            source: "qrc:/gallery/assets/panda.png"
            fillMode: Image.PreserveAspectFit
            renderMode: imageDemo.modes[imageDemo.modeIndex].mode
        }
    }

    Text {
        text: imageDemo.modes[imageDemo.modeIndex].name + ": " + imageDemo.modes[imageDemo.modeIndex].blurb
        color: "#7ee0ff"
    }

    Text {
        text: picture.status === Image.Ready ? "Source " + picture.sourceSize.width + "x" + picture.sourceSize.height + " px" : picture.status === Image.Error ? "Image failed to load" : "Loading"
        color: "#2cde85"
        bold: true
    }
}
