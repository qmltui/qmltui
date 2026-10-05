import QtQml
import Qtui
import Qtui.Controls
import Qtui.Examples.DeviceCockpit

Rectangle {
    id: page
    color: "transparent"

    property var toasts: null
    readonly property var services: CockpitBackend.services

    onVisibleChanged: {
        if (visible)
            unitTable.focus = true
        else
            filterInput.focus = false
    }

    Keys.onPressed: (event) => {
        // Keys bubbling out of the focused filter input: only leave the field.
        if (filterInput.activeFocus) {
            if (event.key === Qt.Key_Escape) {
                filterInput.focus = false
                unitTable.focus = true
                event.accepted = true
            }
            return
        }
        if (event.key === Qt.Key_Slash) {
            filterInput.focus = true
            event.accepted = true
        } else if (event.key === Qt.Key_S) {
            page.confirmAction("start")
            event.accepted = true
        } else if (event.key === Qt.Key_X) {
            page.confirmAction("stop")
            event.accepted = true
        } else if (event.key === Qt.Key_R) {
            page.confirmAction("restart")
            event.accepted = true
        }
    }

    function selectedUnit() {
        const list = page.services.units
        if (unitTable.selectedRow < 0 || unitTable.selectedRow >= list.length)
            return ""
        return list[unitTable.selectedRow].unit
    }

    function confirmAction(action) {
        if (!page.services.canManage)
            return
        const unit = page.selectedUnit()
        if (unit === "")
            return
        confirmDialog.action = action
        confirmDialog.unit = unit
        confirmDialog.title = action.toUpperCase() + " SERVICE"
        confirmDialog.message = action.charAt(0).toUpperCase() + action.slice(1)
                + " " + unit + "?"
        confirmDialog.open()
    }

    Connections {
        target: page.services
        function onActionFinished(unit, action, ok, message) {
            if (page.toasts)
                page.toasts.showToast(ok ? "Service " + action : "Action failed",
                                      message, ok ? "success" : "error")
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            width: parent.width
            height: 3
            color: Theme.bgPanel
            border.style: filterInput.activeFocus ? "double" : "single"
            border.color: filterInput.activeFocus ? Theme.borderFocus : Theme.borderNormal

            Row {
                anchors.fill: parent
                anchors.margins: 1
                anchors.leftMargin: 2
                spacing: 1

                Text { text: "FILTER [/]:"; color: Theme.textAccent; bold: true }
                TextInput {
                    id: filterInput
                    width: Math.max(10, parent.width - 32)
                    color: Theme.textPrimary
                    placeholderText: "type to match unit or description"
                    placeholderColor: Theme.textDim
                    onTextChanged: page.services.filter = text
                    onAccepted: {
                        filterInput.focus = false
                        unitTable.focus = true
                    }
                }
                StatusBadge {
                    status: page.services.canManage ? "success" : "warning"
                    text: page.services.canManage ? "MANAGED" : "READ-ONLY"
                    showIcon: false
                }
            }
        }

        Rectangle {
            width: parent.width
            height: parent.height - 4
            border.style: "single"
            border.color: Theme.borderNormal

            TableView {
                id: unitTable
                anchors.fill: parent
                anchors.margins: 1
                model: page.services.units
                headerColor: Theme.textAccent
                headerBgColor: Theme.bgPanel
                selectionColor: Theme.textPrimary
                selectionBgColor: Theme.bgActiveTab

                columns: [
                    { title: "UNIT", key: "unit", width: 30 },
                    { title: "ACTIVE", key: "active", width: 9 },
                    { title: "SUB", key: "sub", width: 9 },
                    { title: "DESCRIPTION", key: "description", width: 46 }
                ]

                cellFormat: (row, col, value) => {
                    if (col === 1) {
                        if (value === "active")
                            return { text: value, color: Theme.statusOk, bold: true }
                        if (value === "failed")
                            return { text: value, color: Theme.statusError, bold: true }
                        return { text: value, color: Theme.textDim }
                    }
                    if (col === 3)
                        return { text: value, color: Theme.textSecondary }
                    return { text: value }
                }
            }
        }

        Text {
            text: page.services.canManage
                  ? "[↑↓] Select  [S] Start  [X] Stop  [R] Restart  [/] Filter"
                  : "[↑↓] Select  [/] Filter  — service manager unavailable, read-only"
            color: Theme.textDim
        }
    }

    ConfirmDialog {
        id: confirmDialog

        property string action: ""
        property string unit: ""

        onAccepted: {
            if (action === "start")
                page.services.startUnit(unit)
            else if (action === "stop")
                page.services.stopUnit(unit)
            else
                page.services.restartUnit(unit)
        }
    }
}
