pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Image

// Image: Renders bitmaps into terminal cells via Braille, HalfBlock, ASCII,
// or native terminal graphics protocols (Kitty / Sixel in Image.Auto mode).
ColumnLayout {
    id: imageDemo

    readonly property string defaultSource: "qrc:/gallery/assets/bike-picsum.jpg"
    readonly property var modes: [
        {
            mode: Image.Auto,
            name: "Auto",
            blurb: "kitty or sixel where supported, else half-block"
        },
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
        }
    ]
    property int modeIndex: 0

    function loadSource() {
        const next = sourceInput.text.trim();
        if (next.length === 0)
            sourceInput.text = imageDemo.defaultSource;
        picture.source = sourceInput.text.trim();
    }

    anchors.fill: parent
    spacing: 1
    focusPolicy: Qt.StrongFocus
    focus: true

    Keys.onPressed: event => {
        if (!sourceInput.activeFocus && event.key === Qt.Key_M) {
            imageDemo.modeIndex = (imageDemo.modeIndex + 1) % imageDemo.modes.length;
            event.accepted = true;
        }
    }

    Text {
        text: "Press M to cycle renderMode · Tab or click to edit source (Enter to load)."
        color: "#8b949e"
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 1

        Text {
            text: "Source:"
            color: "#8b949e"
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: sourceInput.activeFocus ? "#122d24" : "#161b22"

            TextInput {
                id: sourceInput
                anchors.fill: parent
                text: imageDemo.defaultSource
                placeholderText: "Enter image URL or file path..."
                color: "#c9d1d9"
                onAccepted: {
                    imageDemo.loadSource();
                    imageDemo.focus = true;
                }
                onActiveFocusChanged: {
                    if (!activeFocus)
                        imageDemo.loadSource();
                }
            }
        }
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
            smooth: false
            source: imageDemo.defaultSource
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
        color: picture.status === Image.Error ? "#f87171" : "#2cde85"
        bold: true
    }
}
