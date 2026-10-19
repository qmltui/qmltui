pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// TabBar: Horizontal tab selector driving a StackLayout via currentIndex.
Column {
    spacing: 1

    Text {
        text: "Left/Right or click switches tabs; StackLayout follows currentIndex."
        color: "#8b949e"
    }

    TabBar {
        id: tabs
        width: Math.min(44, parent.width)
        focus: true
        tabs: ["Overview", "Telemetry", "Logs"]
        selectionColor: "#fbbf24"
        selectedTextColor: "#0d1117"
    }

    // Each child Item of StackLayout corresponds to one tab index
    StackLayout {
        width: Math.min(44, parent.width)
        height: 4
        currentIndex: tabs.currentIndex

        Column {
            Text {
                text: "3 services healthy, 0 degraded"
                color: "#2cde85"
            }

            Text {
                text: "Uptime 14d 02:11"
                color: "#c9d1d9"
            }
        }

        Column {
            Text {
                text: "cpu 42%   mem 61%   net 3.2 MB/s"
                color: "#7ee0ff"
            }

            Text {
                text: "p99 latency 38 ms"
                color: "#c9d1d9"
            }
        }

        Column {
            Text {
                text: "12:00:01 INFO  gateway ready"
                color: "#c9d1d9"
            }

            Text {
                text: "12:00:04 WARN  retrying upstream"
                color: "#fbbf24"
            }
        }
    }
}
