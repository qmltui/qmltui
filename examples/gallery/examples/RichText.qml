pragma ComponentBehavior: Bound
import QmlTui

// RichText: Word-wrapped rich text with HTML tags, ANSI SGR escapes, and clickable links.
Column {
    id: demo

    property string link: "none yet"

    anchors.centerIn: parent
    spacing: 1

    Rectangle {
        width: 54
        height: 8
        color: "#161b22"

        // RichText / Text.RichText parses HTML tags and dispatches onLinkActivated
        Text {
            anchors {
                fill: parent
                margins: 1
            }
            wrapMode: Text.WordWrap
            textFormat: Text.RichText
            text: "<b>Text</b> wraps long content and understands HTML tags: <font color='#00ff66'>colours</font>, <i>emphasis</i> and links such as <a href='https://doc.qt.io/qt-6/qml-tutorial.html'>the QML tutorial</a> or <a href='https://github.com/qmltui-dev/qmltui'>the QmlTui repository</a>. Click a link to activate it."
            onLinkActivated: url => {
                demo.link = url;
            }
        }
    }

    Text {
        text: "Last link: " + demo.link
        color: "#2cde85"
    }
}
