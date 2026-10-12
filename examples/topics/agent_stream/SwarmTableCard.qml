import QtQml
import QmlTui

Column {
    id: swarmCard
    required property string title
    property var tableModel: null

    width: parent ? parent.width : 76
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: "✦ Subagent Swarm Mesh:"
            color: "#38bdf8"
            bold: true
        }
        Text {
            text: " " + (swarmCard.title || "Audit Results")
            color: "white"
            bold: true
        }
    }

    TableView {
        width: parent.width > 0 ? Math.min(parent.width, 76) : 76
        height: 5
        headerColor: "#38bdf8"
        headerBackgroundColor: "transparent"
        showColumnGrid: false
        alternatingRows: false

        columns: [
            { title: "Subagent", width: 18, align: "left" },
            { title: "Scope", width: 20, align: "left" },
            { title: "Status", width: 10, align: "center" },
            { title: "Findings", width: 26, align: "left" }
        ]

        model: swarmCard.tableModel ? swarmCard.tableModel : [
            ["MemoryAuditor", "src/QmlTui/engine/", "Pass", "0 leaks (checked 18)"],
            ["QmlProfiler", "examples/agent/", "Pass", "0 binding loops"],
            ["RaceVerifier", "Terminal.cpp", "Pass", "Thread-affinity safe"]
        ]

        cellFormat: (row, col, value) => {
            if (col === 0) return { text: value, color: "#a78bfa", bold: true }
            if (col === 1) return { text: value, color: "#94a3b8" }
            if (col === 2) return { text: "✔ " + value, color: "#34d399", bold: true }
            if (col === 3) return { text: value, color: "#e2e8f0" }
            return { text: value }
        }
    }
}
