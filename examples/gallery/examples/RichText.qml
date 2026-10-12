pragma ComponentBehavior: Bound
import QmlTui

// RichText: Wrap, rich text, links. One tile of the QmlTui gallery (examples/gallery)
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
        id: demo
        x: 2
        y: 1
        property string link: "none yet"
        spacing: 1
        Rectangle {
            width: 66
            height: 7
            border.style: Border.Single
            border.color: "#fde68a"
            Text {
                anchors.fill: parent
                anchors.margins: 1
                wrapMode: Text.WordWrap
                textFormat: Text.RichText
                text: "<b>Text</b> wraps long content to its width and understands HTML tags: <font color='#7ee0ff'>colours</font>, <i>emphasis</i> and links such as <a href='https://doc.qt.io/qt-6/qml-tutorial.html'>the QML tutorial</a> or <a href='https://github.com/qmltui-dev/qmltui'>the QmlTui repository</a>. Click a link to activate it."
                onLinkActivated: url => demo.link = url
            }
        }
        Text { text: "Last link: " + demo.link; color: "#fde68a" }
    }
}
