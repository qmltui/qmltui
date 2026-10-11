import QtQml
import Qamui
import Qamui.Controls

Column {
    id: sysCard
    required property string title
    required property string content

    readonly property var lines: sysCard.content.split("\n")

    width: parent ? parent.width : 80
    spacing: 0

    Text {
        visible: sysCard.title.length > 0
        text: "✦ " + sysCard.title
        color: "#38bdf8"
        bold: true
    }

    Repeater {
        model: sysCard.lines
        delegate: Text {
            required property string modelData
            text: "  " + modelData
            color: "#e2e8f0"
        }
    }

    // Context capacity bar if it's a cost/metrics card
    Row {
        visible: sysCard.title.toLowerCase().indexOf("cost") !== -1 || sysCard.title.toLowerCase().indexOf("usage") !== -1
        spacing: 1

        Text {
            text: "  • Context Window:  "
            color: "#e2e8f0"
        }
        ProgressBar {
            value: 0.18
            from: 0.0
            to: 1.0
            style: "segmented"
            color: "#34d399"
            width: 14
        }
        Text {
            text: " 23.4k / 128k (18%)"
            color: "#94a3b8"
        }
    }

    // Live latency sparkline if it's a cost/metrics card
    Row {
        visible: sysCard.title.toLowerCase().indexOf("cost") !== -1 || sysCard.title.toLowerCase().indexOf("usage") !== -1
        spacing: 1

        Text {
            text: "  • Peak Rate:       "
            color: "#e2e8f0"
        }
        Sparkline {
            values: [14, 22, 28, 35, 42, 58, 62, 54, 70, 85, 92, 88]
            color: "#38bdf8"
            width: 12
            height: 1
        }
        Text {
            text: " 92 tok/s (avg: 64 tok/s)"
            color: "#94a3b8"
        }
    }

    // Multi-line Throughput Stream Waveform
    Column {
        visible: sysCard.title.toLowerCase().indexOf("cost") !== -1 || sysCard.title.toLowerCase().indexOf("usage") !== -1
        width: parent.width
        spacing: 0

        Text {
            text: "  • Throughput Stream Waveform (10s window):"
            color: "#e2e8f0"
        }
        Text {
            text: "    120 ┤          ⡠⠔⠊⠉⠒⢄        ⡠⠊⠑⢄"
            color: "#38bdf8"
        }
        Text {
            text: "     80 ┤     ⡠⠒⠁        ⠑⢄    ⡠⠊   ⠑⢄"
            color: "#38bdf8"
        }
        Text {
            text: "     40 ┤⢀⠤⠊⠁             ⠑⠢⠔⠊      ⠑⠤⢄"
            color: "#38bdf8"
        }
        Text {
            text: "      0 └──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴──┴─ (t)"
            color: "#64748b"
        }
    }
}
