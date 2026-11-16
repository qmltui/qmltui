pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Terminal {
    id: root

    property int currentYear: 2026
    property date todayDate: new Date()
    property date selectedDate: new Date()
    property string statusMessage: "Select a day or type DD/MM/YYYY"

    property var notesMap: {
        const map = {};
        const now = new Date();
        const todayKey = root.formatDateKey(now);
        const tomorrow = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1);
        map[todayKey] = ["Review release checklist", "Run test suite"];
        map[root.formatDateKey(tomorrow)] = ["Tag release candidate"];
        map["2026-08-18"] = ["Milestone review"];
        map["2026-12-25"] = ["Holiday release freeze"];
        return map;
    }

    readonly property var currentNotesList: {
        const list = root.notesMap[root.formatDateKey(root.selectedDate)];
        return list ? list : [];
    }

    readonly property int totalNotesCount: {
        let count = 0;
        const keys = Object.keys(root.notesMap);
        for (let i = 0; i < keys.length; ++i) {
            if (keys[i].startsWith(root.currentYear + "-"))
                count += root.notesMap[keys[i]].length;
        }
        return count;
    }

    function padZero(num: int): string {
        return num < 10 ? "0" + num : "" + num;
    }

    function formatDateKey(d): string {
        if (!d)
            return "";
        return d.getFullYear() + "-" + padZero(d.getMonth() + 1) + "-" + padZero(d.getDate());
    }

    function formatShortDate(d): string {
        if (!d)
            return "";
        return padZero(d.getDate()) + "/" + padZero(d.getMonth() + 1) + "/" + d.getFullYear();
    }

    function getRelativeDateLabel(d): string {
        if (!d)
            return "";
        const now = new Date();
        const today = new Date(now.getFullYear(), now.getMonth(), now.getDate());
        const target = new Date(d.getFullYear(), d.getMonth(), d.getDate());
        const diffDays = Math.round((target.getTime() - today.getTime()) / 86400000);
        if (diffDays === 0)
            return "TODAY";
        if (diffDays === 1)
            return "+1 DAY";
        if (diffDays === -1)
            return "-1 DAY";
        return (diffDays > 0 ? "+" : "") + diffDays + " DAYS";
    }

    function selectDate(d) {
        if (!d)
            return;
        root.selectedDate = new Date(d.getTime());
        cal.selectedDate = root.selectedDate;
        root.currentYear = root.selectedDate.getFullYear();
        const count = (root.notesMap[root.formatDateKey(root.selectedDate)] || []).length;
        root.statusMessage = "Selected " + root.formatShortDate(root.selectedDate) + " (" + count + " notes)";
    }

    function shiftYear(delta: int) {
        root.currentYear += delta;
        const m = root.selectedDate.getMonth();
        const maxDay = new Date(root.currentYear, m + 1, 0).getDate();
        selectDate(new Date(root.currentYear, m, Math.min(root.selectedDate.getDate(), maxDay)));
        root.statusMessage = "Year " + root.currentYear;
    }

    function goToToday() {
        const now = new Date();
        root.todayDate = now;
        selectDate(now);
        root.statusMessage = "Jumped to Today (" + root.formatShortDate(now) + ")";
    }

    function jumpToTypedDate(str: string): bool {
        const parts = str.trim().split(/[\/\-\.]/);
        if (parts.length < 2) {
            root.statusMessage = "Invalid date. Use DD/MM/YYYY";
            return false;
        }
        const day = parseInt(parts[0], 10);
        const month = parseInt(parts[1], 10) - 1;
        let year = parts.length >= 3 ? parseInt(parts[2], 10) : root.currentYear;
        if (year < 100)
            year += 2000;
        const maxDays = new Date(year, month + 1, 0).getDate();
        if (isNaN(day) || isNaN(month) || isNaN(year) || month < 0 || month > 11 || day < 1 || day > maxDays) {
            root.statusMessage = "Invalid date. Use DD/MM/YYYY";
            return false;
        }
        const parsed = new Date(year, month, day);
        selectDate(parsed);
        root.statusMessage = "Jumped to " + root.formatShortDate(parsed);
        return true;
    }

    function addNote(dateStr: string, noteText: string) {
        if (!noteText || noteText.trim() === "")
            return;
        const newMap = Object.assign({}, root.notesMap);
        const list = newMap[dateStr] ? newMap[dateStr].slice() : [];
        list.push(noteText.trim());
        newMap[dateStr] = list;
        root.notesMap = newMap;
        root.statusMessage = "Added note to " + dateStr;
    }

    function deleteNote(dateStr: string, index: int) {
        const newMap = Object.assign({}, root.notesMap);
        if (!newMap[dateStr])
            return;
        const list = newMap[dateStr].slice();
        list.splice(index, 1);
        if (list.length === 0)
            delete newMap[dateStr];
        else
            newMap[dateStr] = list;
        root.notesMap = newMap;
        root.statusMessage = "Deleted note";
    }

    title: "CALENDAR & DAY PLANNER"

    Keys.onPressed: event => {
        if (event.key === Qt.Key_Escape) {
            if (jumpInput.activeFocus) {
                if (jumpInput.text !== "")
                    jumpInput.text = "";
                else
                    cal.focus = true;
                event.accepted = true;
                return;
            }
            if (noteInput.activeFocus) {
                if (noteInput.text !== "")
                    noteInput.text = "";
                else
                    cal.focus = true;
                event.accepted = true;
                return;
            }
            Qt.quit();
            event.accepted = true;
            return;
        }
        if (event.key === Qt.Key_Q) {
            if (!jumpInput.activeFocus && !noteInput.activeFocus) {
                Qt.quit();
                event.accepted = true;
            }
            return;
        }
        if (jumpInput.activeFocus || noteInput.activeFocus)
            return;

        if (event.key === Qt.Key_BracketLeft) {
            root.shiftYear(-1);
        } else if (event.key === Qt.Key_BracketRight) {
            root.shiftYear(1);
        } else if (event.key === Qt.Key_T || event.key === Qt.Key_Home) {
            root.goToToday();
        } else {
            return;
        }
        event.accepted = true;
    }

    component CompactButton: Button {
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0
    }

    Rectangle {
        id: headerBar

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 3
        border {
            style: Border.Single
            color: "cyan"
        }

        Row {
            anchors {
                left: parent.left
                leftMargin: 2
                verticalCenter: parent.verticalCenter
            }
            spacing: 2

            Text {
                text: "CALENDAR & DAY PLANNER"
                color: "cyan"
                bold: true
            }

            StatusBadge {
                status: "info"
                text: String(root.currentYear)
                variant: "filled"
                badgeColor: "cyan"
            }
        }

        Row {
            anchors {
                right: parent.right
                rightMargin: 2
                verticalCenter: parent.verticalCenter
            }
            spacing: 2

            Text {
                text: root.totalNotesCount + " notes in " + root.currentYear
                color: "yellow"
            }
        }
    }

    RowLayout {
        anchors {
            top: headerBar.bottom
            bottom: footerBar.top
            left: parent.left
            right: parent.right
        }
        spacing: 0

        Panel {
            Layout.preferredWidth: 38
            Layout.fillHeight: true
            border {
                style: Border.Single
                color: "gray"
            }
            title: Text {
                text: "Calendar (" + root.currentYear + ")"
                color: "cyan"
            }

            Calendar {
                id: cal

                anchors.centerIn: parent
                width: 34
                height: 13
                focus: true
                selectedDate: root.selectedDate
                todayDate: root.todayDate
                onDateSelected: d => root.selectDate(d)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Panel {
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                border {
                    style: Border.Single
                    color: "gray"
                }
                title: Text {
                    text: "Jump & Navigate"
                    color: "cyan"
                }

                Column {
                    anchors {
                        fill: parent
                        leftMargin: 1
                        rightMargin: 1
                    }
                    spacing: 0

                    Row {
                        spacing: 1

                        Text {
                            text: "Date:"
                            color: "yellow"
                            bold: true
                        }

                        TextInput {
                            id: jumpInput

                            width: 12
                            height: 1
                            placeholderText: "DD/MM/YYYY"
                            placeholderColor: "gray"
                            color: "white"
                            onAccepted: {
                                if (root.jumpToTypedDate(text)) {
                                    text = "";
                                    cal.focus = true;
                                }
                            }
                        }

                        CompactButton {
                            text: "[Go]"
                            onClicked: {
                                if (root.jumpToTypedDate(jumpInput.text)) {
                                    jumpInput.text = "";
                                    cal.focus = true;
                                }
                            }
                        }
                    }

                    Row {
                        spacing: 1

                        CompactButton {
                            text: "[< " + (root.currentYear - 1) + "]"
                            onClicked: root.shiftYear(-1)
                        }

                        CompactButton {
                            text: "[ Today ]"
                            onClicked: root.goToToday()
                        }

                        CompactButton {
                            text: "[" + (root.currentYear + 1) + " >]"
                            onClicked: root.shiftYear(1)
                        }
                    }
                }
            }

            Panel {
                Layout.fillWidth: true
                Layout.preferredHeight: 4
                border {
                    style: Border.Single
                    color: "gray"
                }
                title: Text {
                    text: "Selected Date"
                    color: "cyan"
                }

                Column {
                    anchors {
                        fill: parent
                        leftMargin: 1
                        rightMargin: 1
                    }
                    spacing: 0

                    Row {
                        spacing: 1

                        Text {
                            text: root.formatShortDate(root.selectedDate)
                            color: "cyan"
                            bold: true
                        }

                        StatusBadge {
                            text: root.getRelativeDateLabel(root.selectedDate)
                            variant: "filled"
                            badgeBackgroundColor: text === "TODAY" ? "green" : "cyan"
                        }
                    }

                    Text {
                        text: root.currentNotesList.length + " scheduled note(s)"
                        color: "gray"
                    }
                }
            }

            Panel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                border {
                    style: Border.Single
                    color: "gray"
                }
                title: Text {
                    text: "Day Notes & Reminders"
                    color: "cyan"
                }

                Column {
                    anchors {
                        fill: parent
                        leftMargin: 1
                        rightMargin: 1
                    }
                    spacing: 0

                    Row {
                        spacing: 1

                        Text {
                            text: "+"
                            color: "cyan"
                            bold: true
                        }

                        TextInput {
                            id: noteInput

                            width: 22
                            height: 1
                            placeholderText: "Add note..."
                            placeholderColor: "gray"
                            color: "white"
                            onAccepted: {
                                if (text.trim() !== "") {
                                    root.addNote(root.formatDateKey(root.selectedDate), text);
                                    text = "";
                                    cal.focus = true;
                                }
                            }
                        }

                        CompactButton {
                            text: "[Add]"
                            onClicked: {
                                if (noteInput.text.trim() !== "") {
                                    root.addNote(root.formatDateKey(root.selectedDate), noteInput.text);
                                    noteInput.text = "";
                                    cal.focus = true;
                                }
                            }
                        }
                    }

                    Repeater {
                        model: root.currentNotesList
                        delegate: Row {
                            required property int index
                            required property string modelData

                            spacing: 1

                            Text {
                                width: 28
                                text: "• " + modelData
                            }

                            CompactButton {
                                text: "[x]"
                                onClicked: root.deleteNote(root.formatDateKey(root.selectedDate), index)
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: footerBar

        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }
        height: 3
        border {
            style: Border.Single
            color: "gray"
        }

        Row {
            anchors {
                left: parent.left
                leftMargin: 2
                verticalCenter: parent.verticalCenter
            }
            spacing: 1

            Text {
                text: "STATUS: "
                color: "gray"
                bold: true
            }

            Text {
                text: root.statusMessage
                color: "cyan"
                bold: true
            }
        }
    }
}
