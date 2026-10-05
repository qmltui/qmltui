import QtQml
import Qtui
import Qtui.Controls
import Qtui.Extras

Terminal {
    id: root
    fillTerminal: true
    useAlternateScreen: true
    title: "Qtui Year Calendar & Day Planner"

    property int currentYear: 2026
    property date todayDate: new Date()
    property date selectedDate: new Date()
    property int activeThemeIndex: 0
    property string statusMessage: "Select a day or type DD/MM/YYYY"
    property int columnMode: 0 // 0 = Auto, 2 = 2 Cols, 3 = 3 Cols, 4 = 4 Cols

    readonly property int gridColumns: {
        if (columnMode === 2) return 2
        if (columnMode === 3) return 3
        if (columnMode === 4) return 4
        if (yearGridPanel.width >= 90) return 4
        if (yearGridPanel.width >= 68) return 3
        return 2
    }

    readonly property int gridRows: Math.ceil(12 / gridColumns)

    readonly property var themes: [
        {
            name: "Cyber Cyan",
            accent: "cyan",
            today: "magenta",
            header: "cyan",
            title: "yellow",
            border: "cyan",
            selectedBg: "cyan",
            selectedText: "black",
            noteColor: "yellow",
            panelBorder: "cyan"
        },
        {
            name: "Amber Glow",
            accent: "#ffaa00",
            today: "#ffff55",
            header: "#ffaa00",
            title: "#ffcc00",
            border: "#ffaa00",
            selectedBg: "#ffaa00",
            selectedText: "black",
            noteColor: "#ffff55",
            panelBorder: "#ffaa00"
        },
        {
            name: "Emerald Matrix",
            accent: "#50fa7b",
            today: "#f1fa8c",
            header: "#50fa7b",
            title: "#8be9fd",
            border: "#50fa7b",
            selectedBg: "#50fa7b",
            selectedText: "black",
            noteColor: "#50fa7b",
            panelBorder: "#50fa7b"
        },
        {
            name: "Nord Frost",
            accent: "#88c0d0",
            today: "#ebcb8b",
            header: "#81a1c1",
            title: "#eceff4",
            border: "#88c0d0",
            selectedBg: "#88c0d0",
            selectedText: "#2e3440",
            noteColor: "#ebcb8b",
            panelBorder: "#88c0d0"
        }
    ]

    readonly property var currentTheme: themes[activeThemeIndex]

    // Notes store: { "YYYY-MM-DD": ["Note 1", "Note 2", ...] }
    property var notesMap: {
        var map = {}
        var now = new Date()
        var todayKey = root.formatDateKey(now)
        var tomorrow = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1)
        var tomorrowKey = root.formatDateKey(tomorrow)

        map[todayKey] = [
            "Team Sprint Planning & Standup",
            "Review Year Calendar & Planner"
        ]
        map[tomorrowKey] = [
            "Architecture sync with core team"
        ]
        map["2026-08-18"] = ["Qtui v2.0 Release Meeting"]
        map["2026-08-25"] = ["Performance profiling & benchmark"]
        map["2026-09-01"] = ["Monthly roadmap alignment"]
        map["2026-09-06"] = [
            "Team Sprint Planning & Standup",
            "Review Year Calendar & Planner"
        ]
        map["2026-10-31"] = ["Halloween Hackathon"]
        map["2026-12-25"] = ["Holiday Celebration"]
        map["2026-12-31"] = ["New Year's Eve Retrospective"]

        return map
    }

    readonly property var monthNames: [
        "January", "February", "March", "April",
        "May", "June", "July", "August",
        "September", "October", "November", "December"
    ]

    readonly property var shortMonthNames: [
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    ]

    readonly property var dayNames: ["Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"]

    function padZero(num) {
        return num < 10 ? "0" + num : "" + num
    }

    function formatDateKey(d) {
        if (!d) return ""
        return d.getFullYear() + "-" + padZero(d.getMonth() + 1) + "-" + padZero(d.getDate())
    }

    function formatShortDate(d) {
        if (!d) return ""
        return padZero(d.getDate()) + "/" + padZero(d.getMonth() + 1) + "/" + d.getFullYear()
    }

    function formatFullDate(d) {
        if (!d) return ""
        var days = ["Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"]
        return days[d.getDay()] + ", " + monthNames[d.getMonth()] + " " + d.getDate() + ", " + d.getFullYear()
    }

    function getRelativeDateLabel(d) {
        if (!d) return ""
        var now = new Date()
        var today = new Date(now.getFullYear(), now.getMonth(), now.getDate())
        var target = new Date(d.getFullYear(), d.getMonth(), d.getDate())
        var diffMs = target.getTime() - today.getTime()
        var diffDays = Math.round(diffMs / (1000 * 60 * 60 * 24))
        if (diffDays === 0) return "TODAY"
        if (diffDays === 1) return "+1 DAY"
        if (diffDays === -1) return "-1 DAY"
        if (diffDays > 1) return "+" + diffDays + " DAYS"
        return diffDays + " DAYS"
    }

    function getWeekNumber(d) {
        if (!d) return 1
        var date = new Date(d.getTime())
        date.setHours(0, 0, 0, 0)
        date.setDate(date.getDate() + 3 - (date.getDay() + 6) % 7)
        var week1 = new Date(date.getFullYear(), 0, 4)
        return 1 + Math.round(((date.getTime() - week1.getTime()) / 86400000 - 3 + (week1.getDay() + 6) % 7) / 7)
    }

    function getQuarter(d) {
        if (!d) return "Q1"
        return "Q" + (Math.floor(d.getMonth() / 3) + 1)
    }

    function isSameDate(d1, d2) {
        if (!d1 || !d2) return false
        return d1.getFullYear() === d2.getFullYear() &&
               d1.getMonth() === d2.getMonth() &&
               d1.getDate() === d2.getDate()
    }

    function selectDate(d) {
        if (!d) return
        root.selectedDate = new Date(d.getTime())
        root.currentYear = root.selectedDate.getFullYear()
        var count = (root.notesMap[root.formatDateKey(root.selectedDate)] || []).length
        root.statusMessage = "Selected " + root.formatShortDate(root.selectedDate) + " (" + count + " note" + (count === 1 ? "" : "s") + ")"
    }

    function moveSelection(deltaDays) {
        var curr = root.selectedDate ? root.selectedDate : new Date()
        var target = new Date(curr.getFullYear(), curr.getMonth(), curr.getDate() + deltaDays)
        selectDate(target)
    }

    function prevYear() {
        root.currentYear--
        root.selectDate(new Date(root.currentYear, root.selectedDate.getMonth(), Math.min(root.selectedDate.getDate(), new Date(root.currentYear, root.selectedDate.getMonth() + 1, 0).getDate())))
        root.statusMessage = "Year " + root.currentYear
    }

    function nextYear() {
        root.currentYear++
        root.selectDate(new Date(root.currentYear, root.selectedDate.getMonth(), Math.min(root.selectedDate.getDate(), new Date(root.currentYear, root.selectedDate.getMonth() + 1, 0).getDate())))
        root.statusMessage = "Year " + root.currentYear
    }

    function goToToday() {
        var now = new Date()
        root.todayDate = now
        selectDate(now)
        root.statusMessage = "Jumped to Today (" + root.formatShortDate(now) + ")"
    }

    function parseDateInput(str) {
        if (!str) return null
        var parts = str.trim().split(/[\/\-\.]/)
        if (parts.length < 2) return null
        var day = parseInt(parts[0], 10)
        var month = parseInt(parts[1], 10) - 1
        var year = parts.length >= 3 ? parseInt(parts[2], 10) : root.currentYear
        if (year < 100) year += 2000
        if (isNaN(day) || isNaN(month) || isNaN(year)) return null
        if (month < 0 || month > 11) return null
        var maxDays = new Date(year, month + 1, 0).getDate()
        if (day < 1 || day > maxDays) return null
        return new Date(year, month, day)
    }

    function jumpToTypedDate(str) {
        var parsed = parseDateInput(str)
        if (parsed) {
            selectDate(parsed)
            root.statusMessage = "Jumped to " + root.formatShortDate(parsed)
            return true
        } else {
            root.statusMessage = "Invalid date. Use DD/MM/YYYY"
            return false
        }
    }

    function addNote(dateStr, noteText) {
        if (!noteText || noteText.trim() === "") return
        var newMap = Object.assign({}, root.notesMap)
        var list = newMap[dateStr] ? newMap[dateStr].slice() : []
        list.push(noteText.trim())
        newMap[dateStr] = list
        root.notesMap = newMap
        root.statusMessage = "Added note to " + dateStr
    }

    function deleteNote(dateStr, index) {
        var newMap = Object.assign({}, root.notesMap)
        if (newMap[dateStr]) {
            var list = newMap[dateStr].slice()
            list.splice(index, 1)
            if (list.length === 0) delete newMap[dateStr]
            else newMap[dateStr] = list
            root.notesMap = newMap
            root.statusMessage = "Deleted note"
        }
    }

    function clearNotes(dateStr) {
        var newMap = Object.assign({}, root.notesMap)
        if (newMap[dateStr]) {
            delete newMap[dateStr]
            root.notesMap = newMap
            root.statusMessage = "Cleared notes for " + dateStr
        }
    }

    readonly property var currentNotesList: {
        var dateKey = root.formatDateKey(root.selectedDate)
        var list = root.notesMap[dateKey]
        return list ? list : []
    }

    readonly property int totalNotesCount: {
        var count = 0
        var keys = Object.keys(root.notesMap)
        for (var i = 0; i < keys.length; ++i) {
            if (keys[i].startsWith(root.currentYear + "-")) {
                count += root.notesMap[keys[i]].length
            }
        }
        return count
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            if (jumpInput.activeFocus || noteInput.activeFocus) {
                jumpInput.focus = false
                noteInput.focus = false
                event.accepted = true
                return
            }
            Qt.quit()
            event.accepted = true
            return
        }

        if (jumpInput.activeFocus || noteInput.activeFocus) {
            return
        }

        if (event.key === Qt.Key_Left || event.key === Qt.Key_H) {
            root.moveSelection(-1)
            event.accepted = true
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
            root.moveSelection(1)
            event.accepted = true
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
            root.moveSelection(-7)
            event.accepted = true
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
            root.moveSelection(7)
            event.accepted = true
        } else if (event.key === Qt.Key_BracketLeft) {
            root.prevYear()
            event.accepted = true
        } else if (event.key === Qt.Key_BracketRight) {
            root.nextYear()
            event.accepted = true
        } else if (event.key === Qt.Key_T || event.key === Qt.Key_Home) {
            root.goToToday()
            event.accepted = true
        } else if (event.key === Qt.Key_C) {
            if (root.columnMode === 0) root.columnMode = 3
            else if (root.columnMode === 3) root.columnMode = 4
            else if (root.columnMode === 4) root.columnMode = 2
            else root.columnMode = 0
            var modeStr = root.columnMode === 0 ? "Auto (" + root.gridColumns + ")" : ("" + root.columnMode)
            root.statusMessage = "Grid: " + modeStr + " columns"
            event.accepted = true
        } else if (event.key === Qt.Key_1) {
            root.activeThemeIndex = 0
            root.statusMessage = "Theme: Cyber Cyan"
            event.accepted = true
        } else if (event.key === Qt.Key_2) {
            root.activeThemeIndex = 1
            root.statusMessage = "Theme: Amber Glow"
            event.accepted = true
        } else if (event.key === Qt.Key_3) {
            root.activeThemeIndex = 2
            root.statusMessage = "Theme: Emerald Matrix"
            event.accepted = true
        } else if (event.key === Qt.Key_4) {
            root.activeThemeIndex = 3
            root.statusMessage = "Theme: Nord Frost"
            event.accepted = true
        }
    }

    // Top Header Bar
    Rectangle {
        id: headerBar
        height: 3
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        border.style: "double"
        border.color: root.currentTheme.border

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            Text {
                text: "📅 YEAR CALENDAR & PLANNER"
                color: root.currentTheme.accent
                bold: true
            }

            StatusBadge {
                status: "info"
                text: "" + root.currentYear
                variant: "filled"
                badgeColor: root.currentTheme.accent
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            Text {
                text: root.totalNotesCount + " notes in " + root.currentYear
                color: "yellow"
            }

            StatusBadge {
                status: "success"
                text: root.currentTheme.name
                variant: "subtle"
                badgeColor: root.currentTheme.accent
            }
        }
    }

    // Main Workspace
    Item {
        id: workspace
        anchors.top: headerBar.bottom
        anchors.bottom: footerBar.top
        anchors.left: parent.left
        anchors.right: parent.right

        // Left Pane: 12-Month Year Calendar Grid
        Panel {
            id: yearGridPanel
            title: "Year " + root.currentYear + " (12 Months Overview)"
            titleColor: root.currentTheme.accent
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: {
                if (root.columnMode === 3) return Math.min(parent.width - 20, 70)
                if (root.columnMode === 4) return Math.min(parent.width - 20, 92)
                if (root.columnMode === 2) return Math.min(parent.width - 20, 48)
                if (parent.width >= 124) return parent.width - 32
                if (parent.width >= 98) return parent.width - 30
                return Math.max(48, parent.width - 32)
            }
            border.style: "single"
            border.color: root.currentTheme.panelBorder

            ScrollView {
                anchors.fill: parent
                anchors.margins: 1
                focusable: true

                Column {
                    width: parent.width
                    spacing: 1

                    Repeater {
                        model: root.gridRows
                        delegate: Row {
                            required property int index
                            property int rowIdx: index
                            spacing: 2

                            Repeater {
                                model: root.gridColumns
                                delegate: Rectangle {
                                    required property int index
                                    property int colIdx: index
                                    property int monthIdx: rowIdx * root.gridColumns + colIdx
                                    property bool isMonthValid: monthIdx < 12

                                    visible: isMonthValid
                                    width: 21
                                    height: 8
                                    color: "transparent"
                                    border.style: "none"

                                    Column {
                                        anchors.fill: parent
                                        spacing: 0

                                        // Month Name Header
                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: "── " + root.monthNames[monthIdx] + " ──"
                                            color: (root.selectedDate.getMonth() === monthIdx && root.selectedDate.getFullYear() === root.currentYear) ? root.currentTheme.accent : root.currentTheme.title
                                            bold: true
                                        }

                                        // Day Names Header
                                        Text {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: "Su Mo Tu We Th Fr Sa"
                                            color: root.currentTheme.header
                                        }

                                        // 6 Weeks of Days Grid
                                        Column {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            spacing: 0

                                            Repeater {
                                                model: 6
                                                delegate: Row {
                                                    required property int index
                                                    property int weekIdx: index
                                                    spacing: 1

                                                    Repeater {
                                                        model: 7
                                                        delegate: Rectangle {
                                                            required property int index
                                                            property int dayOfWeekIdx: index
                                                            property int cellIdx: weekIdx * 7 + dayOfWeekIdx

                                                            property int firstDayOffset: new Date(root.currentYear, monthIdx, 1).getDay()
                                                            property int daysInThisMonth: new Date(root.currentYear, monthIdx + 1, 0).getDate()
                                                            property int dayNum: cellIdx - firstDayOffset + 1
                                                            property bool isValidDay: dayNum >= 1 && dayNum <= daysInThisMonth

                                                            property bool isSelected: isValidDay &&
                                                                                      root.selectedDate.getFullYear() === root.currentYear &&
                                                                                      root.selectedDate.getMonth() === monthIdx &&
                                                                                      root.selectedDate.getDate() === dayNum
                                                            property bool isToday: isValidDay &&
                                                                                   root.todayDate.getFullYear() === root.currentYear &&
                                                                                   root.todayDate.getMonth() === monthIdx &&
                                                                                   root.todayDate.getDate() === dayNum
                                                            property bool hasNotes: isValidDay &&
                                                                                    ((root.notesMap[root.currentYear + "-" + root.padZero(monthIdx + 1) + "-" + root.padZero(dayNum)] || []).length > 0)

                                                            width: 2
                                                            height: 1
                                                            color: isSelected ? root.currentTheme.selectedBg : "transparent"

                                                            Text {
                                                                anchors.fill: parent
                                                                horizontalAlignment: Qt.AlignRight
                                                                visible: isValidDay
                                                                text: isValidDay ? (dayNum < 10 ? (" " + dayNum) : ("" + dayNum)) : ""
                                                                color: {
                                                                    if (isSelected) return root.currentTheme.selectedText
                                                                    if (isToday) return root.currentTheme.today
                                                                    if (hasNotes) return root.currentTheme.noteColor
                                                                    return "white"
                                                                }
                                                                bold: isSelected || isToday || hasNotes
                                                            }

                                                            MouseArea {
                                                                anchors.fill: parent
                                                                enabled: isValidDay
                                                                onClicked: {
                                                                    if (isValidDay) {
                                                                        root.selectDate(new Date(root.currentYear, monthIdx, dayNum))
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // Right Pane: Date Jump, Overview, & Notes Planner
        Column {
            id: rightCol
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: yearGridPanel.right
            anchors.right: parent.right
            spacing: 0

            // 1. Quick Date Jump & Year Navigation Panel (Height: 4)
            Panel {
                title: "Jump & Navigate"
                titleColor: root.currentTheme.accent
                width: rightCol.width
                height: 4
                border.style: "single"
                border.color: root.currentTheme.panelBorder

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                    // Jump to DD/MM/YYYY text input
                    Row {
                        spacing: 0

                        Text {
                            text: "Date: ["
                            color: "yellow"
                            bold: true
                        }

                        Rectangle {
                            width: 12
                            height: 1
                            color: jumpInput.activeFocus ? "#1e3a5f" : "#0f172a"
                            border.style: "none"

                            TextInput {
                                id: jumpInput
                                anchors.fill: parent
                                placeholderText: "DD/MM/YYYY"
                                placeholderColor: "gray"
                                color: "white"
                                focusable: true

                                onAccepted: {
                                    if (root.jumpToTypedDate(text)) {
                                        text = ""
                                        jumpInput.focus = false
                                    }
                                }
                            }
                        }

                        Text {
                            text: "] "
                            color: "yellow"
                            bold: true
                        }

                        Button {
                            label: "[Go]"
                            implicitWidth: 6
                            implicitHeight: 1
                            onClicked: {
                                if (root.jumpToTypedDate(jumpInput.text)) {
                                    jumpInput.text = ""
                                    jumpInput.focus = false
                                }
                            }
                        }
                    }

                    // Year switcher and Today buttons
                    Row {
                        spacing: 1

                        Button {
                            label: "[< " + (root.currentYear - 1) + "]"
                            implicitWidth: 9
                            implicitHeight: 1
                            onClicked: root.prevYear()
                        }

                        Button {
                            label: "[ Today ]"
                            implicitWidth: 9
                            implicitHeight: 1
                            onClicked: root.goToToday()
                        }

                        Button {
                            label: "[" + (root.currentYear + 1) + " >]"
                            implicitWidth: 9
                            implicitHeight: 1
                            onClicked: root.nextYear()
                        }
                    }
                }
            }

            // 2. Selected Date Card (Height: 4)
            Panel {
                id: selectedDateCard
                title: "Selected Date"
                titleColor: root.currentTheme.accent
                width: rightCol.width
                height: 4
                border.style: "single"
                border.color: root.currentTheme.panelBorder

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                    Row {
                        spacing: 1

                        Text {
                            text: "🗓 " + root.formatShortDate(root.selectedDate)
                            color: root.currentTheme.accent
                            bold: true
                        }

                        StatusBadge {
                            text: root.getRelativeDateLabel(root.selectedDate)
                            variant: "filled"
                            bold: true
                            badgeBgColor: {
                                var rel = root.getRelativeDateLabel(root.selectedDate)
                                if (rel === "TODAY") return "green"
                                if (rel === "+1 DAY" || rel === "-1 DAY") return "cyan"
                                return "gray"
                            }
                        }
                    }

                    Text {
                        text: "  Wk " + root.getWeekNumber(root.selectedDate) +
                              " • " + root.getQuarter(root.selectedDate) +
                              " • " + root.currentNotesList.length + " note" + (root.currentNotesList.length === 1 ? "" : "s")
                        color: "gray"
                    }
                }
            }

            // 3. Day Notes Manager & Editor (Height: Remaining)
            Panel {
                title: "Day Notes & Reminders"
                titleColor: root.currentTheme.accent
                width: rightCol.width
                height: Math.max(8, rightCol.height - 8)
                border.style: "single"
                border.color: root.currentTheme.panelBorder

                Column {
                    anchors.fill: parent
                    anchors.margins: 1
                    spacing: 0

                    // Add Note Input Field
                    Row {
                        spacing: 0

                        Text {
                            text: "+ ["
                            color: root.currentTheme.accent
                            bold: true
                        }

                        Rectangle {
                            width: Math.max(10, rightCol.width - 13)
                            height: 1
                            color: noteInput.activeFocus ? "#1e3a5f" : "#0f172a"
                            border.style: "none"

                            TextInput {
                                id: noteInput
                                anchors.fill: parent
                                placeholderText: "Add note..."
                                placeholderColor: "gray"
                                color: "white"
                                focusable: true

                                onAccepted: {
                                    if (text.trim() !== "") {
                                        root.addNote(root.formatDateKey(root.selectedDate), text)
                                        text = ""
                                    }
                                }
                            }
                        }

                        Text {
                            text: "] "
                            color: root.currentTheme.accent
                            bold: true
                        }

                        Button {
                            label: "[Add]"
                            implicitWidth: 7
                            implicitHeight: 1
                            onClicked: {
                                if (noteInput.text.trim() !== "") {
                                    root.addNote(root.formatDateKey(root.selectedDate), noteInput.text)
                                    noteInput.text = ""
                                }
                            }
                        }
                    }

                    Text {
                        text: "──────────────────────────────"
                        color: "#3e4756"
                    }

                    // Notes List
                    ScrollView {
                        width: parent.width
                        height: Math.max(3, parent.height - 2)
                        focusable: true

                        Column {
                            width: parent.width
                            spacing: 0

                            // Notes list
                            Repeater {
                                model: root.currentNotesList
                                delegate: Row {
                                    required property int index
                                    required property string modelData
                                    spacing: 0

                                    Text {
                                        text: "• " + modelData
                                        color: "white"
                                        width: Math.max(10, rightCol.width - 7)
                                    }

                                    Button {
                                        label: "[x]"
                                        implicitWidth: 3
                                        implicitHeight: 1
                                        onClicked: root.deleteNote(root.formatDateKey(root.selectedDate), index)
                                    }
                                }
                            }

                            // Empty state
                            Text {
                                visible: root.currentNotesList.length === 0
                                text: "No notes for this date.\nType above & press Enter!"
                                color: "gray"
                            }
                        }
                    }
                }
            }
        }
    }

    // Bottom Status & Hotkey Bar
    Rectangle {
        id: footerBar
        height: 3
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        border.style: "double"
        border.color: root.currentTheme.border

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1

            Text {
                text: "STATUS: "
                color: "gray"
                bold: true
            }

            Text {
                text: root.statusMessage
                color: root.currentTheme.accent
                bold: true
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            spacing: 1

            Text {
                text: "[C] Cols (" + root.gridColumns + ")  [T] Today  [1-4] Theme  [Q] Quit"
                color: "white"
            }
        }
    }
}
