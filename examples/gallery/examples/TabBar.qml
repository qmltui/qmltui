pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// TabBar: Tabs over a StackLayout. One tile of the QmlTui gallery (examples/gallery)
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
        x: 2
        y: 1
        spacing: 1
        Text { text: "Left/Right or a click switches tabs; the StackLayout follows currentIndex."; color: "#6e7b8b" }
        TabBar {
            id: tabs
            width: 60
            focus: true
            tabs: ["Overview", "Telemetry", "Logs"]
            selectionColor: "#fbbf24"
            selectedTextColor: "#0b0f14"
        }
        StackLayout {
            width: 60
            height: 4
            currentIndex: tabs.currentIndex
            Column {
                Text { text: "3 services healthy, 0 degraded"; color: "#65e6a0" }
                Text { text: "Uptime 14d 02:11"; color: "#c9d1d9" }
            }
            Column {
                Text { text: "cpu 42%   mem 61%   net 3.2 MB/s"; color: "#7ee0ff" }
                Text { text: "p99 latency 38 ms"; color: "#c9d1d9" }
            }
            Column {
                Text { text: "12:00:01 INFO  gateway ready"; color: "#c9d1d9" }
                Text { text: "12:00:04 WARN  retrying upstream"; color: "#fbbf24" }
            }
        }
    }
}
