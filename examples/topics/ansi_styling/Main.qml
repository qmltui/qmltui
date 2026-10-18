pragma ComponentBehavior: Bound
import QtQml
import QmlTui

Terminal {
    id: rootWindow
    property string lastActivatedLink: "none"

    color: "black"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit();
            event.accepted = true;
        } else if (event.key === Qt.Key_M) {
            rootWindow.mouseReporting = !rootWindow.mouseReporting;
            event.accepted = true;
        }
    }

    Column {
        anchors {
            fill: parent
            margins: 1
        }
        spacing: 0

        // Header
        Rectangle {
            width: parent.width
            height: 3
            color: "#222222"

            Text {
                anchors.fill: parent
                text: "Concept Demo: Text Styling & ANSI Parser"
                color: "yellow"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Section 1: Explicit QML properties
        Text {
            width: parent.width
            text: "1. Explicit Text Properties:"
            color: "cyan"
            underline: true
        }

        Row {
            spacing: 2
            width: parent.width

            Text {
                width: 15
                text: "Bold Text"
                color: "green"
                bold: true
            }

            Text {
                width: 15
                text: "Italic Text"
                color: "green"
                italic: true
            }

            Text {
                width: 18
                text: "Underline Text"
                color: "green"
                underline: true
            }

            Text {
                width: 15
                text: "Reversed Text"
                color: "green"
                reversed: true
            }
        }

        // Section 2: ANSI escape sequences
        Text {
            width: parent.width
            text: "\n2. Inline ANSI Escape Sequence Parsing:"
            color: "cyan"
            underline: true
        }

        Text {
            width: parent.width
            text: "16-Color: \u001b[1;31mBold Red\u001b[0m, \u001b[4;32mUnderline Green\u001b[0m, \u001b[7;33mReversed Yellow\u001b[0m"
        }

        Text {
            width: parent.width
            text: "Truecolor SGR: \u001b[38;2;255;165;0mOrange Foreground (255, 165, 0)\u001b[0m"
        }

        Text {
            width: parent.width
            text: "Background SGR: \u001b[48;2;128;0;128;38;2;255;255;255mWhite text on Purple Background\u001b[0m"
        }

        // Section 3: HTML rich text tags
        Text {
            width: parent.width
            text: "\n3. HTML Rich Text Tag Parsing (Option 1):"
            color: "cyan"
            underline: true
        }

        Text {
            width: parent.width
            text: "Bold & Italic: <b>Bold text</b>, <i>Italic text</i>, <b><i>Bold Italic</i></b>"
        }

        Text {
            width: parent.width
            text: "Colors (Hex & Names): <font color='orange'>Orange</font>, <font color='#ff00ff'>Magenta Hex</font>, <u><font color='lime'>Underline Lime</font></u>"
        }

        Text {
            width: parent.width
            text: "Reversed Attribute: <reversed><font color='yellow'>Reversed Yellow Text</font></reversed>"
        }

        // Section 4: Hyperlinks (OSC 8 & linkActivated)
        Text {
            width: parent.width
            text: "\n4. Terminal Hyperlinks (OSC 8 Browser Links & In-App Clicks):"
            color: "cyan"
            underline: true
        }

        Text {
            width: parent.width
            textFormat: Text.RichText
            text: "HTML & Markdown: <a href=\"https://qt.io\">Qt Project</a> | [QmlTui Docs](https://qmltui.org)"
            onLinkActivated: link => {
                rootWindow.lastActivatedLink = link;
            }
        }

        Text {
            width: parent.width
            text: rootWindow.mouseReporting ? "Mouse reporting ('m' to toggle): ON  -> Ctrl+Shift+Hover/Click in terminal, or click in-app" : "Mouse reporting ('m' to toggle): OFF -> Ctrl+Hover/Click in terminal"
            color: "green"
        }

        Text {
            width: parent.width
            text: "Last activated link: " + rootWindow.lastActivatedLink
            color: "yellow"
        }

        Text {
            width: parent.width
            text: "\nPress 'q' or 'Escape' to exit this example."
            color: "silver"
        }
    }
}
