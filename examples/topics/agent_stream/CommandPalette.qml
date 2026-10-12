pragma ComponentBehavior: Bound

import QtQml
import QmlTui
import QmlTui.Controls

Column {
    id: commandPalette

    Theme { id: theme }

    property string filterText: ""
    property int selectedIndex: 0

    signal commandSelected(string cmd)
    signal commandExecuted(string cmd)

    readonly property var allCommands: [
        { "cmd": "/help",   "desc": "Show all workflows & shortcuts" },
        { "cmd": "/demo 1", "desc": "Refactor InlineRenderer & clamp bounds" },
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

    // Keyboard navigation moves the window; mouse selection keeps the
    // command under the pointer stationary until it is clicked.
    property int windowStart: 0

    width: parent ? parent.width : 80
    spacing: 0
    visible: false

    function open() { visible = true }
    function close() { visible = false }

    function selectNext() {
        if (filteredCommands.length > 0) {
            selectedIndex = (selectedIndex + 1) % filteredCommands.length
            revealCurrent()
        }
    }

    function selectPrev() {
        if (filteredCommands.length > 0) {
            selectedIndex = (selectedIndex - 1 + filteredCommands.length) % filteredCommands.length
            revealCurrent()
        }
    }

    function revealCurrent() {
        windowStart = Math.max(0, Math.min(selectedIndex - 2, filteredCommands.length - 5))
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
            Qt.callLater(() => {
                commandExecuted(chosen)
            })
        }
    }

    onFilteredCommandsChanged: {
        windowStart = 0
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
        // Keep row objects alive when navigation shifts the window.
        // Replacing an array model here would delete the active MouseArea
        // synchronously, while its QML signal handler is still running.
        model: Math.min(5, commandPalette.filteredCommands.length)
        delegate: Rectangle {
            id: commandEntry
            required property int index
            readonly property int actualIdx: commandPalette.windowStart + commandEntry.index
            readonly property var itemData: commandPalette.filteredCommands[commandEntry.actualIdx]
            readonly property bool isCurrent: commandEntry.actualIdx === commandPalette.selectedIndex
            readonly property bool isHovered: cmdMouse.containsMouse

            width: commandPalette.width
            height: 1
            color: commandEntry.isHovered ? (cmdMouse.pressed ? "#334155" : "#1e293b") : (commandEntry.isCurrent ? "#1e293b" : "transparent")

            Row {
                anchors.fill: parent
                spacing: 1

                Text {
                    text: (commandEntry.isCurrent || commandEntry.isHovered) ? "  ❯ " : "    "
                    color: commandEntry.isHovered ? "white" : theme.cyan
                    bold: commandEntry.isCurrent || commandEntry.isHovered
                }

                Text {
                    text: commandEntry.itemData.cmd
                    color: (commandEntry.isCurrent || commandEntry.isHovered) ? "white" : theme.cyan
                    bold: commandEntry.isCurrent || commandEntry.isHovered
                    underline: commandEntry.isHovered
                }

                Text {
                    text: "· " + commandEntry.itemData.desc
                    color: commandEntry.isHovered ? theme.white : (commandEntry.isCurrent ? theme.slate : theme.muted)
                }
            }

            MouseArea {
                id: cmdMouse
                anchors.fill: parent
                hoverEnabled: true
                onEntered: commandPalette.selectedIndex = commandEntry.actualIdx
                onMouseXChanged: if (cmdMouse.containsMouse) commandPalette.selectedIndex = commandEntry.actualIdx
                onMouseYChanged: if (cmdMouse.containsMouse) commandPalette.selectedIndex = commandEntry.actualIdx
                onClicked: {
                    commandPalette.selectedIndex = commandEntry.actualIdx
                    commandPalette.executeCurrent()
                }
            }
        }
    }
}
