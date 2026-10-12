pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls

// Button: Enter, Space or click; background and contentItem restyle it. One tile of the Qamui gallery (examples/gallery)
// and a complete app on its own: run it, or paste it into the playground.
Terminal {
    id: root
    color: "#0b0f14"

    // Q or Escape quits.
    Keys.onPressed: event => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape)
            Qt.quit()
    }

    Column {
        id: buttonDemo
        x: 2
        y: 1
        property int clicks: 0
        spacing: 1
        Text { text: "Tab moves focus; Enter, Space or a click activates; the pointer highlights."; color: "#6e7b8b" }
        Row {
            id: buttons
            spacing: 2
            // hoverEnabled is opt-in: it turns on any-motion mouse reporting
            // for the whole app. The label goes bold under the pointer, flips
            // to reverse video while the mouse button is held, and the double
            // border marks keyboard focus, which a click also moves here.
            Button { text: "Primary"; focus: true; hoverEnabled: true; onClicked: buttonDemo.clicks++ }
            Button { text: "Secondary"; hoverEnabled: true; onClicked: buttonDemo.clicks++ }
            Button {
                background: Rectangle {
                    color: "#65e6a0"
                    border.color: "default"
                    border.style: Border.None
                }
                text: "Filled"
                textColor: "#0b0f14"
                hoverEnabled: true
                onClicked: buttonDemo.clicks++
            }
            Button { text: "Reset"; textColor: "#f87171"; hoverEnabled: true; onClicked: buttonDemo.clicks = 0 }
        }
        Row {
            spacing: 2
            // checkable: activation toggles `checked` before clicked().
            Button {
                id: pin
                text: pin.checked ? "Pinned" : "Pin"
                checkable: true
                hoverEnabled: true
                highlighted: pin.checked
            }
            // A background that reacts to the press: `down` follows the mouse.
            Button {
                id: reactive
                text: "Hold me"
                hoverEnabled: true
                background: Rectangle {
                    color: reactive.down ? "#65e6a0" : "#1d2733"
                    border.color: reactive.activeFocus ? "#65e6a0" : "#3a4657"
                    border.style: reactive.down ? Border.Double : Border.Single
                }
            }
            // A contentItem of your own: an icon glyph before the label.
            Button {
                id: withIcon
                text: "Save"
                hoverEnabled: true
                contentItem: Row {
                    spacing: 1
                    Text { text: "💾"; }
                    Text { text: withIcon.text; color: "#7ee0ff"; bold: withIcon.activeFocus || withIcon.hovered }
                }
            }
        }
        Text {
            text: buttonDemo.clicks === 0 ? "No clicks yet" : "Clicked " + buttonDemo.clicks + " time" + (buttonDemo.clicks === 1 ? "" : "s")
            color: "#65e6a0"
            bold: true
        }
        Text {
            readonly property Item under: buttons.children.find(b => b.hovered) ?? null
            text: under ? "Hovering " + under.text : "Hover a button to highlight it"
            color: "#6e7b8b"
        }
    }
}
