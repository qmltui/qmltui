import QtQuick
import Qtui
import Qtui.Controls

Column {
    id: diffCard
    required property string title

    property bool collapsed: false

    width: parent ? parent.width : 80
    spacing: 0

    Row {
        spacing: 1
        Text {
            text: diffCard.collapsed ? "▶ Patch:" : "▼ Patch:"
            color: "#38bdf8"
            bold: true
        }
        Text {
            text: " " + diffCard.title
            color: "white"
            bold: true
        }
        Text {
            text: diffCard.collapsed ? " · [Collapsed - Click to expand / F3 for side-by-side]" : " · (+3, -1 lines) [F3 for side-by-side]"
            color: "#64748b"
        }
    }

    Column {
        visible: !diffCard.collapsed
        width: parent.width
        spacing: 0

        Text {
            text: "  @@ -72,6 +72,9 @@ void StreamRenderer::resize(...) @@"
            color: "#a78bfa"
        }
        Text {
            text: "  -    m_promptBuffer.resize(width, m_lastPromptHeight);"
            color: "#f87171"
        }
        Text {
            text: "  +    const int safeHeight = std::max(0, m_lastPromptHeight);"
            color: "#34d399"
        }
        Text {
            text: "  +    m_promptBuffer.resize(width, safeHeight);"
            color: "#34d399"
        }
        Text {
            text: "  +    markDirty();"
            color: "#34d399"
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: diffCard.collapsed = !diffCard.collapsed
    }
}
