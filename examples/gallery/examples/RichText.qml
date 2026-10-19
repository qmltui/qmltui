pragma ComponentBehavior: Bound
import QmlTui

// RichText: Word-wrapped rich text with HTML tags, ANSI SGR escapes, and clickable links.
Column {
    id: demo

    property string link: "none yet"

    spacing: 1

    Rectangle {
        width: Math.min(54, parent.width)
        height: 8
        border {
            style: Border.Single
            color: "#fde68a"
        }

        // RichText / Text.RichText parses HTML tags and dispatches onLinkActivated
        Text {
            anchors {
                fill: parent
                margins: 1
            }
            wrapMode: Text.WordWrap
            textFormat: Text.RichText
            text: "<b>Text</b> wraps long content and understands HTML tags: <font color='#7ee0ff'>colours</font>, <i>emphasis</i> and links such as <a href='https://doc.qt.io/qt-6/qml-tutorial.html'>the QML tutorial</a> or <a href='https://github.com/yosri/qml-tui'>the QmlTui repository</a>. Click a link to activate it."
            onLinkActivated: url => {
                demo.link = url;
            }
        }
    }

    Text {
        text: "Last link: " + demo.link
        color: "#fde68a"
    }
}
