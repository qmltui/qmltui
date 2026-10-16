pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property bool syntaxEnabled: true
    property int highlightedSpans: 0
    readonly property int lineCount: editor.text.length === 0 ? 1 : editor.text.split("\n").length

    function applyHighlights() {
        if (!root.syntaxEnabled) {
            editor.clearHighlightLayer("syntax");
            root.highlightedSpans = 0;
            return;
        }
        const src = editor.text;
        const ranges = [];
        const rules = [
            {
                word: "import",
                color: "#ff7b72",
                bold: true
            },
            {
                word: "QmlTui",
                color: "#7ee0ff",
                bold: true
            },
            {
                word: "Terminal",
                color: "#2cde85",
                bold: true
            },
            {
                word: "Panel",
                color: "#c084fc",
                bold: true
            },
            {
                word: "Text",
                color: "#38bdf8",
                bold: true
            },
            {
                word: "true",
                color: "#fbbf24",
                bold: false
            },
            {
                word: "\"Stage\"",
                color: "#a3e635",
                bold: false
            }
        ];
        for (let r = 0; r < rules.length; ++r) {
            const rule = rules[r];
            let pos = 0;
            while (pos < src.length) {
                const idx = src.indexOf(rule.word, pos);
                if (idx < 0)
                    break;
                ranges.push({
                    start: idx,
                    length: rule.word.length,
                    color: rule.color,
                    bold: rule.bold
                });
                pos = idx + rule.word.length;
            }
        }
        editor.setHighlightLayer("syntax", ranges, editor.textRevision, 1);
        root.highlightedSpans = ranges.length;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "TextArea · Highlight Layer Editor"
            titleColor: "#f8fafc"
            titleBadge: "[CORE]"
            titleBadgeColor: "#a78bfa"
            titleTrailing: "rev:" + editor.textRevision
            titleTrailingColor: "#8b949e"
            footer: "Type multi-line QML · Tab inspector · Esc quit"
            footerColor: "#8b949e"
            footerTrailing: editor.readOnly ? "READ-ONLY" : "EDITABLE"
            footerTrailingColor: editor.readOnly ? "#fbbf24" : "#2cde85"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            TextArea {
                id: editor
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                color: "#c9d1d9"
                placeholderText: "Enter QML snippet..."
                text: "import QmlTui\n\nTerminal {\n    Panel {\n        title: \"Stage\"\n        Text {\n            bold: true\n        }\n    }\n}"

                onTextChanged: root.applyHighlights()
                Component.onCompleted: root.applyHighlights()
            }
        }

        Panel {
            Layout.preferredWidth: 26
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            footer: "TextArea API"
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
                    text: "EDITOR STATE"
                    bold: true
                    color: "#a78bfa"
                }

                Text {
                    text: "cursorPos: " + editor.cursorPosition
                    color: "#c9d1d9"
                }

                Text {
                    text: "revision:  " + editor.textRevision
                    color: "#38bdf8"
                }

                Text {
                    text: "lines:     " + root.lineCount
                    color: "#f8fafc"
                }

                Text {
                    text: "canUndo:   " + (editor.canUndo ? "true" : "false")
                    color: editor.canUndo ? "#2cde85" : "#8b949e"
                }

                Text {
                    text: "readOnly:  " + (editor.readOnly ? "true" : "false")
                    color: editor.readOnly ? "#fbbf24" : "#2cde85"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "HIGHLIGHT LAYER"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "layer: \"syntax\""
                    color: "#c9d1d9"
                }

                Text {
                    text: "spans: " + root.highlightedSpans
                    color: "#2cde85"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "CONTROLS"
                    bold: true
                    color: "#58a6ff"
                }

                CheckBox {
                    text: "readOnly"
                    checked: editor.readOnly
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        editor.readOnly = checked;
                    }
                }

                CheckBox {
                    text: "Syntax layer"
                    checked: root.syntaxEnabled
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.syntaxEnabled = checked;
                        root.applyHighlights();
                    }
                }

                Button {
                    width: parent.width
                    height: 1
                    text: "[ Undo Edit ]"
                    textColor: "#38bdf8"
                    onClicked: {
                        editor.undo();
                        editor.focus = true;
                    }
                }
            }
        }
    }
}
