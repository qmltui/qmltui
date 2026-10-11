pragma ComponentBehavior: Bound
import Qamui

// Paragraph: Wrap, rich text, links. One tile of the Qamui gallery (examples/apps/gallery)
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
        id: paragraphDemo
        x: 2
        y: 1
        property string link: "none yet"
        spacing: 1
        Rectangle {
            width: 66
            height: 7
            border.style: "single"
            border.color: "#fde68a"
            Paragraph {
                anchors.fill: parent
                anchors.margins: 1
                wrapMode: true
                textFormat: Paragraph.RichText
                text: "<b>Paragraph</b> wraps long text to its width and understands a subset of HTML: <font color='#7ee0ff'>colours</font>, <i>emphasis</i> and links such as <a href='https://doc.qt.io/qt-6/qml-tutorial.html'>the QML tutorial</a> or <a href='https://github.com/qamui-dev/qamui'>the Qamui repository</a>. Click a link to activate it."
                onLinkActivated: url => paragraphDemo.link = url
            }
        }
        Text { text: "Last link: " + paragraphDemo.link; color: "#fde68a" }
    }
}
