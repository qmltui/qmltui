pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// LogView: Append-only tail log. One tile of the QmlTui gallery (examples/gallery)
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
        id: logDemo
        x: 2
        y: 1
        property int lines: 0
        readonly property var levels: ["INFO", "INFO", "INFO", "WARN", "INFO", "DEBUG", "ERROR"]
        readonly property var messages: ["gateway ready", "request served in 12 ms", "cache warm", "retrying upstream", "job queued", "gc pause 3 ms", "upstream timeout"]
        spacing: 1
        Timer {
            interval: 400
            running: true
            repeat: true
            onTriggered: {
                const i = logDemo.lines % logDemo.levels.length
                const stamp = ("00" + Math.floor(logDemo.lines / 60) % 60).slice(-2) + ":" + ("00" + logDemo.lines % 60).slice(-2)
                log.appendLine("12:" + stamp + " " + logDemo.levels[i].padEnd(5) + " " + logDemo.messages[i])
                logDemo.lines++
            }
        }
        Text { text: "appendLine() adds a row; followTail keeps the newest visible; maximumLineCount drops the oldest."; color: "#6e7b8b" }
        Rectangle {
            width: 60
            height: 8
            border.style: Border.Single
            border.color: "#a3e635"
            LogView {
                id: log
                anchors.fill: parent
                anchors.margins: 1
                followTail: true
                maximumLineCount: 200
                color: "#c9d1d9"
            }
        }
        Text { text: logDemo.lines + " lines appended, at most 200 kept"; color: "#a3e635" }
    }
}
