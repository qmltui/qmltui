pragma ComponentBehavior: Bound
import QtQml
import Qamui
import Qamui.Controls

// Sparkline: Live series in one row. One tile of the Qamui gallery (examples/apps/gallery)
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
        id: sparkDemo
        x: 2
        y: 1
        property var cpu: []
        property var net: []
        property int tick: 0
        spacing: 1
        Component.onCompleted: {
            let c = [], n = []
            for (let i = 0; i < 60; ++i) {
                c.push(45 + Math.sin(i * 0.25) * 25 + Math.random() * 8)
                n.push(Math.max(0, 20 + Math.cos(i * 0.4) * 15 + Math.random() * 10))
            }
            cpu = c
            net = n
        }
        Timer {
            interval: 120
            running: true
            repeat: true
            onTriggered: {
                sparkDemo.tick++
                let c = Array.from(sparkDemo.cpu)
                c.shift()
                c.push(Math.min(100, Math.max(0, 45 + Math.sin(sparkDemo.tick * 0.25) * 25 + Math.random() * 8)))
                sparkDemo.cpu = c
                let n = Array.from(sparkDemo.net)
                n.shift()
                n.push(Math.max(0, 20 + Math.cos(sparkDemo.tick * 0.4) * 15 + Math.random() * 10))
                sparkDemo.net = n
            }
        }
        Text { text: "The newest value is the rightmost column; a Timer appends one every 120 ms."; color: "#6e7b8b" }
        Row {
            spacing: 2
            Text { width: 10; text: "cpu %"; color: "#c9d1d9" }
            Sparkline { width: 60; height: 1; values: sparkDemo.cpu; from: 0; to: 100; color: "#22d3ee" }
            Text { text: sparkDemo.cpu.length ? sparkDemo.cpu[sparkDemo.cpu.length - 1].toFixed(0) : ""; color: "#22d3ee" }
        }
        Row {
            spacing: 2
            Text { width: 10; text: "net MB/s"; color: "#c9d1d9" }
            Sparkline { width: 60; height: 4; filled: true; values: sparkDemo.net; color: "#65e6a0" }
        }
    }
}
