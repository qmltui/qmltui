pragma ComponentBehavior: Bound

import QtQml
import Qtui
import Qtui.Controls

Column {
    id: commandPalette

    Theme { id: theme }

    property string filterText: ""
    property int selectedIndex: 0

    signal commandSelected(string cmd)
    signal commandExecuted(string cmd)

    readonly property var allCommands: [
        { "cmd": "/help",   "desc": "Show all workflows & shortcuts" },
        { "cmd": "/demo 1", "desc": "Refactor StreamRenderer & clamp bounds" },
        { "cmd": "/demo 2", "desc": "Dispatch parallel subagent swarm (TableView)" },
        { "cmd": "/demo 3", "desc": "Security sandbox exploit audit" },
        { "cmd": "/demo 4", "desc": "Explore workspace hierarchy (TreeView)" },
        { "cmd": "/model",  "desc": "Switch active neural model (F2)" },
        { "cmd": "/swarm",  "desc": "Show subagent swarm status (TableView)" },
        { "cmd": "/tree",   "desc": "Inspect workspace hierarchy (TreeView)" },
        { "cmd": "/diff",   "desc": "Inspect side-by-side patch diff (F3)" },
        { "cmd": "/cost",   "desc": "Session token metrics & capacity" },
        { "cmd": "/doctor", "desc": "Run system environment diagnostics" },
        { "cmd": "/clear",  "desc": "Clear terminal history" },
        { "cmd": "/quit",   "desc": "Exit studio session" }
    ]

    readonly property var filteredCommands: {
        const query = filterText.trim().toLowerCase()
        if (query.length === 0 || query === "/")
            return allCommands
        const result = []
        for (let i = 0; i < allCommands.length; ++i) {
            const item = allCommands[i]
            if (item.cmd.toLowerCase().indexOf(query) !== -1 || item.desc.toLowerCase().indexOf(query) !== -1) {
                result.push(item)
            }
        }
        return result
    }

    readonly property string currentCommand: (filteredCommands.length > 0 && selectedIndex >= 0 && selectedIndex < filteredCommands.length) ? filteredCommands[selectedIndex].cmd : ""

    // Sliding window of up to 5 items around selectedIndex
    readonly property int windowStart: {
        if (filteredCommands.length <= 5) return 0
        if (selectedIndex < 3) return 0
        if (selectedIndex >= filteredCommands.length - 2) return filteredCommands.length - 5
        return selectedIndex - 2
    }

    readonly property var visibleCommands: {
        const result = []
        const count = Math.min(5, filteredCommands.length)
        for (let i = 0; i < count; ++i) {
            const actualIndex = windowStart + i
            if (actualIndex < filteredCommands.length) {
                result.push({
                    "item": filteredCommands[actualIndex],
                    "actualIndex": actualIndex
                })
            }
        }
        return result
    }

    width: parent ? parent.width : 80
    spacing: 0
    visible: false

    function open() { visible = true }
    function close() { visible = false }

    function selectNext() {
        if (filteredCommands.length > 0)
            selectedIndex = (selectedIndex + 1) % filteredCommands.length
    }

    function selectPrev() {
        if (filteredCommands.length > 0)
            selectedIndex = (selectedIndex - 1 + filteredCommands.length) % filteredCommands.length
    }

    function chooseCurrent() {
        if (currentCommand.length > 0) {
            const chosen = currentCommand
            close()
            commandSelected(chosen)
        }
    }

    function executeCurrent() {
        if (currentCommand.length > 0) {
            const chosen = currentCommand
            close()
            commandExecuted(chosen)
        }
    }

    onFilteredCommandsChanged: {
        selectedIndex = 0
    }

    // Header Info Bar
    Row {
        width: parent.width
        height: 1
        spacing: 1

        StatusBadge {
            text: "Commands " + (commandPalette.filteredCommands.length > 0 ? "[" + (commandPalette.selectedIndex + 1) + "/" + commandPalette.filteredCommands.length + "]" : "")
            status: "info"
            variant: "subtle"
            showIcon: false
        }

        Text {
            text: "· [Tab/→] fill · [Enter] run · [↑/↓] select · [Esc] close"
            color: theme.muted
        }
    }

    // Borderless Command Suggestions
    Repeater {
        model: commandPalette.visibleCommands
        delegate: Rectangle {
            id: commandEntry
            required property int index
            readonly property var entry: commandPalette.visibleCommands[commandEntry.index]
            readonly property var itemData: commandEntry.entry.item
            readonly property int actualIdx: commandEntry.entry.actualIndex
            readonly property bool isCurrent: commandEntry.actualIdx === commandPalette.selectedIndex

            width: commandPalette.width
            height: 1
            color: commandEntry.isCurrent ? "#1e293b" : "transparent"

            Row {
                anchors.fill: parent
                spacing: 1

                Text {
                    text: commandEntry.isCurrent ? "  ❯ " : "    "
                    color: theme.cyan
                    bold: commandEntry.isCurrent
                }

                Text {
                    text: commandEntry.itemData.cmd
                    color: commandEntry.isCurrent ? "white" : theme.cyan
                    bold: commandEntry.isCurrent
                }

                Text {
                    text: "· " + commandEntry.itemData.desc
                    color: commandEntry.isCurrent ? theme.slate : theme.muted
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    commandPalette.selectedIndex = commandEntry.actualIdx
                    commandPalette.executeCurrent()
                }
            }
        }
    }
}
