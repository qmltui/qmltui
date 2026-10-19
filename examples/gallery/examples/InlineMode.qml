pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// InlineMode: Renders at the bottom of the normal terminal buffer while
// Scrollback.print() appends persistent lines above the live interface.
Column {
    id: inlineDemo

    property int printed: 0
    property real load: 0

    width: parent ? parent.width : 80
    spacing: 1

    Scrollback {
        id: history
    }

    NumberAnimation {
        target: inlineDemo
        property: "load"
        from: 0
        to: 100
        duration: 4000
        loops: -1
        running: true
    }

    Text {
        spans: [
            {
                text: " Inline mode ",
                color: "#0d1117",
                backgroundColor: "#38bdf8",
                bold: true
            },
            {
                text: "  UI sits in the normal buffer; printed lines scroll above it",
                color: "#8b949e"
            }
        ]
    }

    Row {
        spacing: 1

        Text {
            width: 8
            text: "load"
            color: "#c9d1d9"
        }

        ProgressBar {
            width: 30
            from: 0
            to: 100
            value: inlineDemo.load
            style: "segmented"
            color: "#38bdf8"
            showPercentage: true
        }
    }

    Row {
        Text {
            text: "❯ "
            color: "#38bdf8"
            bold: true
        }

        TextInput {
            id: prompt
            width: Math.max(20, inlineDemo.width - 4)
            height: 1
            focus: true
            placeholderText: "type a line and press Enter to print into scrollback"
            color: "#c9d1d9"
            onAccepted: {
                if (text.length === 0)
                    return;
                if (history.print("❯ " + text)) {
                    inlineDemo.printed++;
                    text = "";
                }
            }
        }
    }

    Text {
        text: "printed: " + inlineDemo.printed + "   Esc returns to the gallery"
        color: "#8b949e"
    }
}
