pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property int appendedCount: 0
    property int nextSeq: 7
    property string lastAction: "ready"
    readonly property int lineCount: logView.text.length === 0 ? 0 : logView.text.split("\n").length
    readonly property var sampleMessages: [
        {
            level: "INFO",
            msg: "worker: batch #42 committed (8ms)"
        },
        {
            level: "WARN",
            msg: "mem: arena utilization at 86%"
        },
        {
            level: "ERROR",
            msg: "rpc: peer reset on channel 3"
        },
        {
            level: "INFO",
            msg: "gc: reclaimed 128 KiB cell buffers"
        }
    ]

    function appendLevel(level, message) {
        const sec = String(root.nextSeq).padStart(2, "0");
        const tag = level === "INFO" ? "[INFO] " : (level === "WARN" ? "[WARN] " : "[ERROR]");
        logView.appendLine("12:00:" + sec + " " + tag + " " + message);
        root.nextSeq += 1;
        root.appendedCount += 1;
        root.lastAction = "+" + level;
    }

    function appendNextEntry() {
        const item = root.sampleMessages[root.appendedCount % root.sampleMessages.length];
        root.appendLevel(item.level, item.msg);
    }

    function clearLogs() {
        logView.clear();
        root.lastAction = "cleared";
    }

    function highlightLevels() {
        const src = logView.text;
        const ranges = [];
        const tokens = [
            {
                key: "[INFO]",
                color: "#2cde85",
                bold: true
            },
            {
                key: "[WARN]",
                color: "#fbbf24",
                bold: true
            },
            {
                key: "[ERROR]",
                color: "#f87171",
                bold: true
            }
        ];
        for (let t = 0; t < tokens.length; ++t) {
            const tok = tokens[t];
            let pos = 0;
            while (pos < src.length) {
                const idx = src.indexOf(tok.key, pos);
                if (idx < 0)
                    break;
                ranges.push({
                    start: idx,
                    length: tok.key.length,
                    color: tok.color,
                    bold: tok.bold
                });
                pos = idx + tok.key.length;
            }
        }
        logView.setHighlightLayer("severity", ranges, logView.textRevision, 1);
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_A) {
                root.appendNextEntry();
                event.accepted = true;
            } else if (event.key === Qt.Key_C) {
                root.clearLogs();
                event.accepted = true;
            } else if (event.key === Qt.Key_F) {
                logView.followTail = !logView.followTail;
                root.lastAction = logView.followTail ? "tail:on" : "tail:off";
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "LogView · Streaming Log Analyzer"
            titleColor: "#f8fafc"
            titleBadge: "[CORE]"
            titleBadgeColor: "#a3e635"
            titleTrailing: root.lineCount + " lines"
            titleTrailingColor: "#8b949e"
            footer: "A append · C clear · F followTail · ↑↓ scroll"
            footerColor: "#8b949e"
            footerTrailing: logView.followTail ? "TAIL:ON" : "TAIL:OFF"
            footerTrailingColor: logView.followTail ? "#2cde85" : "#fbbf24"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            LogView {
                id: logView
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                followTail: true
                maximumLineCount: 200
                color: "#c9d1d9"

                onTextChanged: root.highlightLevels()
                Component.onCompleted: {
                    logView.appendLine("12:00:01 [INFO]  engine: terminal session ready");
                    logView.appendLine("12:00:02 [INFO]  layout: viewport 84x20 cells");
                    logView.appendLine("12:00:03 [WARN]  cache: glyph table at 82%");
                    logView.appendLine("12:00:04 [INFO]  router: mouse motion active");
                    logView.appendLine("12:00:05 [ERROR] net: upstream timeout (retry 1)");
                    logView.appendLine("12:00:06 [INFO]  net: reconnected in 14ms");
                }
            }
        }

        Panel {
            Layout.preferredWidth: 26
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            footer: "LogView API"
            footerColor: "#8b949e"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 0

                Text {
                    text: "BUFFER TELEMETRY"
                    bold: true
                    color: "#a3e635"
                }

                Text {
                    text: "lines:    " + root.lineCount
                    bold: true
                    color: "#f8fafc"
                }

                Text {
                    text: "maxLines: " + logView.maximumLineCount
                    color: "#c9d1d9"
                }

                Text {
                    text: "contentY: " + logView.contentY
                    color: "#38bdf8"
                }

                Text {
                    text: "appended: " + root.appendedCount
                    color: "#8b949e"
                }

                Text {
                    width: parent.width
                    text: "last:     " + root.lastAction
                    color: "#2cde85"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "STREAM OPTIONS"
                    bold: true
                    color: "#58a6ff"
                }

                CheckBox {
                    text: "followTail (F)"
                    checked: logView.followTail
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        logView.followTail = checked;
                        root.lastAction = checked ? "tail:on" : "tail:off";
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "ACTIONS"
                    bold: true
                    color: "#58a6ff"
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "[ + Append Log (A) ]"
                    textColor: "#2cde85"
                    onClicked: root.appendNextEntry()
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "[ + Warn Entry ]"
                    textColor: "#fbbf24"
                    onClicked: root.appendLevel("WARN", "queue depth high")
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "[ Clear Buffer (C) ]"
                    textColor: "#f87171"
                    onClicked: root.clearLogs()
                }
            }
        }
    }
}
