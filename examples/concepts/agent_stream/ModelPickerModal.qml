import QtQuick
import Qtui
import Qtui.Controls
import Qtui.Extras

Popup {
    id: modelPicker

    Theme { id: theme }

    property string currentModel: "claude-3-7-sonnet"
    property int selectedIndex: 0

    signal modelSelected(string modelId)

    readonly property var models: [
        {
            "id": "claude-3-7-sonnet",
            "name": "Claude 3.7 Sonnet",
            "provider": "Anthropic",
            "context": "128k / 64k thinking",
            "rate": "$3.00/M in · $15.00/M out",
            "badge": "default",
            "badgeStatus": "success",
            "desc": "Hybrid reasoning with adaptive token budget & code synthesis"
        },
        {
            "id": "gpt-4.5-preview",
            "name": "GPT-4.5 Preview",
            "provider": "OpenAI",
            "context": "128k / 32k thinking",
            "rate": "$7.50/M in · $37.50/M out",
            "badge": "frontier",
            "badgeStatus": "info",
            "desc": "Deep reasoning & architecture analysis with high fidelity"
        },
        {
            "id": "gemini-2.0-flash",
            "name": "Gemini 2.0 Flash",
            "provider": "Google",
            "context": "1.0M context window",
            "rate": "$0.10/M in · $0.40/M out",
            "badge": "fastest",
            "badgeStatus": "warning",
            "desc": "Ultra-low latency streaming with 1M token context window"
        },
        {
            "id": "deepseek-r1",
            "name": "DeepSeek R1",
            "provider": "DeepSeek",
            "context": "64k context window",
            "rate": "$0.55/M in · $2.19/M out",
            "badge": "reasoning",
            "badgeStatus": "subtle",
            "desc": "Open-weights reinforced reasoning for math & logic proofs"
        }
    ]

    width: parent ? parent.width : 78
    height: 16
    clip: true
    color: "#0f172a"
    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnOutsideClick

    function selectNext() {
        selectedIndex = (selectedIndex + 1) % models.length
    }

    function selectPrev() {
        selectedIndex = (selectedIndex - 1 + models.length) % models.length
    }

    function chooseCurrent() {
        if (selectedIndex >= 0 && selectedIndex < models.length) {
            modelSelected(models[selectedIndex].id)
            close()
        }
    }

    onVisibleChanged: {
        if (visible) {
            for (let i = 0; i < models.length; ++i) {
                if (models[i].id === currentModel) {
                    selectedIndex = i
                    break
                }
            }
        }
    }

    Column {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        // Header Title
        Row {
            width: parent.width
            height: 1
            spacing: 1

            StatusBadge {
                text: "Models"
                status: "info"
                variant: "subtle"
                showIcon: false
            }

            Text {
                text: "Neural model selector"
                color: "white"
                bold: true
            }

            Text {
                text: " · [1-4/Enter] select · [Esc] close"
                color: theme.muted
            }
        }

        // Divider
        Rectangle {
            width: parent.width
            height: 1
            color: "transparent"
        }

        // Model List Items
        Repeater {
            model: modelPicker.models
            delegate: Rectangle {
                required property int index
                readonly property var m: modelPicker.models[index]
                readonly property bool isSelected: index === modelPicker.selectedIndex
                readonly property bool isActive: m.id === modelPicker.currentModel

                width: parent.width
                height: 3
                color: isSelected ? "#1e293b" : "transparent"

                Column {
                    anchors.fill: parent
                    anchors.margins: 0
                    spacing: 0

                    Row {
                        width: parent.width
                        height: 1
                        spacing: 1

                        Text {
                            text: (isSelected ? "❯ [" : "  [") + (index + 1) + "] "
                            color: isSelected ? theme.cyan : theme.muted
                            bold: isSelected
                        }

                        Text {
                            text: m.name
                            color: isSelected ? "white" : (isActive ? theme.cyan : theme.slate)
                            bold: isSelected || isActive
                        }

                        Text {
                            text: "(" + m.provider + ")"
                            color: theme.muted
                        }

                        StatusBadge {
                            text: m.badge
                            status: m.badgeStatus
                            variant: "subtle"
                            showIcon: false
                        }

                        Text {
                            text: isActive ? "● active" : ""
                            color: theme.green
                            bold: true
                        }
                    }

                    Row {
                        width: parent.width
                        height: 1
                        spacing: 1

                        Text {
                            text: "      " + m.desc
                            color: isSelected ? theme.slate : theme.muted
                        }
                    }

                    Row {
                        width: parent.width
                        height: 1
                        spacing: 1

                        Text {
                            text: "      Ctx: " + m.context + " · " + m.rate
                            color: theme.muted
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        modelPicker.selectedIndex = index
                        modelPicker.chooseCurrent()
                    }
                }
            }
        }
    }

    Keys.priority: Keys.BeforeItem
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_1) {
            selectedIndex = 0
            chooseCurrent()
            event.accepted = true
        } else if (event.key === Qt.Key_2) {
            selectedIndex = 1
            chooseCurrent()
            event.accepted = true
        } else if (event.key === Qt.Key_3) {
            selectedIndex = 2
            chooseCurrent()
            event.accepted = true
        } else if (event.key === Qt.Key_4) {
            selectedIndex = 3
            chooseCurrent()
            event.accepted = true
        } else if (event.key === Qt.Key_Up) {
            selectPrev()
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            selectNext()
            event.accepted = true
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            chooseCurrent()
            event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            close()
            event.accepted = true
        }
    }
}
