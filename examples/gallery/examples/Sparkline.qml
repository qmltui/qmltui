pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

// Sparkline: Compact numeric series chart rendered with Unicode block glyphs.
Column {
    id: sparkDemo

    property var cpu: []
    property var net: []
    property int tick: 0

    spacing: 1

    // Seed deterministic synthetic series on startup
    Component.onCompleted: {
        let c = [], n = [];
        for (let i = 0; i < 40; ++i) {
            c.push(45 + Math.sin(i * 0.25) * 25 + (Math.sin(i * 1.7) + 1) * 4);
            n.push(Math.max(0, 20 + Math.cos(i * 0.4) * 15 + (Math.cos(i * 1.3) + 1) * 5));
        }
        cpu = c;
        net = n;
    }

    // Append a new sample every 120 ms
    Timer {
        interval: 120
        running: true
        repeat: true
        onTriggered: {
            sparkDemo.tick++;
            let c = Array.from(sparkDemo.cpu);
            c.shift();
            c.push(Math.min(100, Math.max(0, 45 + Math.sin(sparkDemo.tick * 0.25) * 25 + (Math.sin((sparkDemo.tick + 39) * 1.7) + 1) * 4)));
            sparkDemo.cpu = c;
            let n = Array.from(sparkDemo.net);
            n.shift();
            n.push(Math.max(0, 20 + Math.cos(sparkDemo.tick * 0.4) * 15 + (Math.cos((sparkDemo.tick + 39) * 1.3) + 1) * 5));
            sparkDemo.net = n;
        }
    }

    Text {
        text: "Newest sample is on the right; Timer appends every 120 ms."
        color: "#8b949e"
    }

    // Single-row sparkline with explicit [from, to] range
    Row {
        spacing: 1

        Text {
            width: 9
            text: "cpu %"
            color: "#c9d1d9"
        }

        Sparkline {
            width: 28
            height: 1
            values: sparkDemo.cpu
            from: 0
            to: 100
            color: "#22d3ee"
        }

        Text {
            text: sparkDemo.cpu.length ? sparkDemo.cpu[sparkDemo.cpu.length - 1].toFixed(0) : ""
            color: "#22d3ee"
        }
    }

    // Multi-row filled area sparkline
    Row {
        spacing: 1

        Text {
            width: 9
            text: "net MB/s"
            color: "#c9d1d9"
        }

        Sparkline {
            width: 28
            height: 4
            filled: true
            values: sparkDemo.net
            color: "#2cde85"
        }
    }
}
