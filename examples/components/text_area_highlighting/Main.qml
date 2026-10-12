pragma ComponentBehavior: Bound
import QtQml
import Qamui

Terminal {
    id: root

    property bool alternatePalette: false
    property bool isQmlMode: false

    readonly property string jsonSample:
`{
  // Server configuration with comments
  "service": "terminal-daemon",
  "port": 8080,
  "timeout": 30.5,
  "enabled": true,
  /* Multiline comment block:
     inspect highlight layers */
  "fallback": null
}`

    readonly property string qmlSample:
`import QtQml
import Qamui

Terminal {
  id: window
  // Interactive terminal interface in pure QML
  width: 80
  height: 24

  Rectangle {
    anchors.fill: parent
    border.style: Border.Single
    border.color: "gray"

    Text {
      anchors.centerIn: parent
      text: "Hello from Qamui QML!"
      color: "cyan"
      bold: true
    }
  }
}`

    property string jsonText: jsonSample
    property string qmlText: qmlSample

    function toggleDocumentMode() {
        if (root.isQmlMode) {
            root.qmlText = editor.text
            root.isQmlMode = false
            editor.text = root.jsonText
        } else {
            root.jsonText = editor.text
            root.isQmlMode = true
            editor.text = root.qmlText
        }
    }

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_F2 || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_T)) {
            root.alternatePalette = !root.alternatePalette
            event.accepted = true
        } else if (event.key === Qt.Key_F3 || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_M)) {
            root.toggleDocumentMode()
            event.accepted = true
        }
    }

    Rectangle {
        anchors.fill: parent
        border.style: Border.Single
        border.color: "gray"

        Column {
            anchors.fill: parent
            anchors.margins: 1

            // Header / Search Bar & Actions
            Row {
                width: parent.width
                height: 1

                Text {
                    text: " Search: "
                    color: "yellow"
                    bold: true
                }
                TextInput {
                    id: searchInput
                    width: 25
                    placeholderText: "search (e.g. timeout)..."
                }
                Text {
                    text: " | Mode: "
                    color: "gray"
                }
                Text {
                    id: modeBtn
                    text: root.isQmlMode ? "[Mode: QML] (F3)" : "[Mode: JSON] (F3)"
                    color: "yellow"
                    underline: true
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.toggleDocumentMode()
                    }
                }
                Text {
                    text: " | Palette: "
                    color: "gray"
                }
                Text {
                    id: restyleBtn
                    text: "[Toggle Palette] (F2)"
                    color: "cyan"
                    underline: true
                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.alternatePalette = !root.alternatePalette
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: "gray"
            }

            // Editor
            TextArea {
                id: editor
                width: parent.width
                height: parent.height - 3
                focus: true
                tabChangesFocus: false
                tabStopDistance: 2
                text: root.jsonSample
            }

            // Status bar
            Rectangle {
                width: parent.width
                height: 1
                color: "default"
                Text {
                    text: " Mode: " + (root.isQmlMode ? "QML (Pure QML Highlighter)" : "JSON (C++ Highlighter)")
                        + " | Rev: " + editor.textRevision
                        + " | [F2: Theme | F3: Mode | Esc: Quit]"
                    color: "gray"
                }
            }
        }
    }

    // C++ Highlighter for JSON documents
    JsonHighlighter {
        id: jsonHighlighter
        target: root.isQmlMode ? null : editor
        stringColor: root.alternatePalette ? "magenta" : "green"
        numberColor: "cyan"
        keywordColor: "blue"
        commentColor: "#888888"
        searchBgColor: "yellow"
        searchTerm: searchInput.text
    }

    // Pure QML Highlighter for QML documents
    QmlHighlighter {
        id: qmlHighlighter
        target: root.isQmlMode ? editor : null
        keywordColor: "blue"
        typeColor: "cyan"
        stringColor: root.alternatePalette ? "magenta" : "green"
        numberColor: "cyan"
        commentColor: "#888888"
        searchBgColor: "yellow"
        searchTerm: searchInput.text
    }
}
