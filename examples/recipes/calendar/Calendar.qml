pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Rectangle {
    id: control

    property date selectedDate: new Date()
    property int year: selectedDate ? selectedDate.getFullYear() : new Date().getFullYear()
    property int month: selectedDate ? selectedDate.getMonth() : new Date().getMonth()
    property int firstDayOfWeek: 0 // 0 = Sunday, 1 = Monday
    property date todayDate: new Date()

    property bool showHeader: true
    property bool showFooter: true
    property bool highlightToday: true

    property string titleColor: "yellow"
    property string dayHeaderColor: "cyan"
    property string normalDayColor: "white"
    property string otherMonthDayColor: "gray"
    property string todayColor: "magenta"
    property string selectedTextColor: "black"
    property string selectedBackgroundColor: "cyan"
    property string focusBorderColor: "yellow"
    property string normalBorderColor: "white"
    property string gridBorderColor: "gray"

    readonly property list<string> monthNames: [qsTr("January"), qsTr("February"), qsTr("March"), qsTr("April"), qsTr("May"), qsTr("June"), qsTr("July"), qsTr("August"), qsTr("September"), qsTr("October"), qsTr("November"), qsTr("December")]
    readonly property list<string> dayNamesSun: [qsTr("Su"), qsTr("Mo"), qsTr("Tu"), qsTr("We"), qsTr("Th"), qsTr("Fr"), qsTr("Sa")]
    readonly property list<string> dayNamesMon: [qsTr("Mo"), qsTr("Tu"), qsTr("We"), qsTr("Th"), qsTr("Fr"), qsTr("Sa"), qsTr("Su")]
    readonly property list<string> currentDayNames: firstDayOfWeek === 1 ? dayNamesMon : dayNamesSun

    signal dateSelected(date selectedDate)

    function daysInMonth(y, m) {
        return new Date(y, m + 1, 0).getDate();
    }

    function firstDayOffset(y, m) {
        const dayOfWeek = new Date(y, m, 1).getDay();
        return (dayOfWeek - firstDayOfWeek + 7) % 7;
    }

    function getCellDate(cellIndex) {
        const offset = firstDayOffset(control.year, control.month);
        const dayNum = cellIndex - offset + 1;
        return new Date(control.year, control.month, dayNum);
    }

    function isSameDate(d1, d2) {
        if (!d1 || !d2)
            return false;
        return d1.getFullYear() === d2.getFullYear() && d1.getMonth() === d2.getMonth() && d1.getDate() === d2.getDate();
    }

    function formatIsoDate(d) {
        if (!d)
            return "";
        const m = d.getMonth() + 1;
        const day = d.getDate();
        return d.getFullYear() + "-" + (m < 10 ? "0" + m : m) + "-" + (day < 10 ? "0" + day : day);
    }

    function selectDate(d) {
        if (!d)
            return;
        control.selectedDate = new Date(d.getTime());
        control.year = control.selectedDate.getFullYear();
        control.month = control.selectedDate.getMonth();
        control.dateSelected(control.selectedDate);
    }

    function moveSelection(deltaDays) {
        const curr = control.selectedDate ? control.selectedDate : new Date();
        const target = new Date(curr.getFullYear(), curr.getMonth(), curr.getDate() + deltaDays);
        selectDate(target);
    }

    function setYearMonth(y, m) {
        let newM = m;
        let newY = y;
        if (newM < 0) {
            newM = 11;
            newY--;
        } else if (newM > 11) {
            newM = 0;
            newY++;
        }
        control.year = newY;
        control.month = newM;
    }

    function prevMonth() {
        setYearMonth(control.year, control.month - 1);
    }

    function nextMonth() {
        setYearMonth(control.year, control.month + 1);
    }

    function prevYear() {
        setYearMonth(control.year - 1, control.month);
    }

    function nextYear() {
        setYearMonth(control.year + 1, control.month);
    }

    function goToToday() {
        const now = new Date();
        control.todayDate = now;
        selectDate(now);
    }

    focusPolicy: enabled ? Qt.StrongFocus : Qt.NoFocus
    implicitWidth: 34
    implicitHeight: showFooter ? 13 : 11
    color: "transparent"
    border {
        style: activeFocus ? Border.Double : Border.Single
        color: !enabled ? "gray" : (activeFocus ? focusBorderColor : normalBorderColor)
    }

    onSelectedDateChanged: {
        if (selectedDate) {
            control.year = selectedDate.getFullYear();
            control.month = selectedDate.getMonth();
        }
    }

    Keys.onPressed: event => {
        if (!control.enabled)
            return;

        if (event.key === Qt.Key_Left || event.key === Qt.Key_H) {
            control.moveSelection(-1);
            event.accepted = true;
        } else if (event.key === Qt.Key_Right || event.key === Qt.Key_L) {
            control.moveSelection(1);
            event.accepted = true;
        } else if (event.key === Qt.Key_Up || event.key === Qt.Key_K) {
            control.moveSelection(-7);
            event.accepted = true;
        } else if (event.key === Qt.Key_Down || event.key === Qt.Key_J) {
            control.moveSelection(7);
            event.accepted = true;
        } else if (event.key === Qt.Key_PageUp) {
            control.prevMonth();
            event.accepted = true;
        } else if (event.key === Qt.Key_PageDown) {
            control.nextMonth();
            event.accepted = true;
        } else if (event.key === Qt.Key_Home || event.key === Qt.Key_T) {
            control.goToToday();
            event.accepted = true;
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
            control.dateSelected(control.selectedDate);
            event.accepted = true;
        }
    }

    Column {
        id: mainColumn

        x: 1
        y: 1
        width: control.width - 2
        spacing: 0

        Row {
            id: headerRow

            visible: control.showHeader
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 1

            Button {
                implicitWidth: 4
                implicitHeight: 1
                text: "<<"
                background: Rectangle {
                    color: "default"
                    border {
                        color: "default"
                        style: Border.None
                    }
                }
                onClicked: control.prevYear()
            }

            Button {
                implicitWidth: 3
                implicitHeight: 1
                text: "<"
                background: Rectangle {
                    color: "default"
                    border {
                        color: "default"
                        style: Border.None
                    }
                }
                onClicked: control.prevMonth()
            }

            Text {
                width: 14
                horizontalAlignment: Qt.AlignHCenter
                text: control.monthNames[control.month] + " " + control.year
                color: control.titleColor
                bold: true
            }

            Button {
                implicitWidth: 3
                implicitHeight: 1
                text: ">"
                background: Rectangle {
                    color: "default"
                    border {
                        color: "default"
                        style: Border.None
                    }
                }
                onClicked: control.nextMonth()
            }

            Button {
                implicitWidth: 4
                implicitHeight: 1
                text: ">>"
                background: Rectangle {
                    color: "default"
                    border {
                        color: "default"
                        style: Border.None
                    }
                }
                onClicked: control.nextYear()
            }
        }

        Row {
            id: dayNamesRow

            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 0

            Repeater {
                model: 7
                delegate: Text {
                    required property int index

                    width: 4
                    horizontalAlignment: Qt.AlignHCenter
                    text: control.currentDayNames[index]
                    color: control.dayHeaderColor
                    bold: true
                }
            }
        }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "────────────────────────────"
            color: control.gridBorderColor
        }

        Column {
            id: gridColumn

            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 0

            Repeater {
                model: 6
                delegate: Row {
                    id: weekRow

                    required property int index
                    property int rowIndex: index

                    spacing: 0

                    Repeater {
                        model: 7
                        delegate: Rectangle {
                            id: dayCell

                            required property int index
                            property int colIndex: index
                            property int cellIndex: weekRow.rowIndex * 7 + colIndex
                            property date cellDate: control.getCellDate(cellIndex)
                            property bool isCurrentMonth: cellDate.getMonth() === control.month
                            property bool isSelected: control.isSameDate(cellDate, control.selectedDate)
                            property bool isToday: control.highlightToday && control.isSameDate(cellDate, control.todayDate)

                            width: 4
                            height: 1
                            color: isSelected ? control.selectedBackgroundColor : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: {
                                    const dayStr = dayCell.cellDate.getDate().toString();
                                    if (dayCell.isSelected)
                                        return dayStr;
                                    if (dayCell.isToday)
                                        return "[" + dayStr + "]";
                                    return dayStr;
                                }
                                color: {
                                    if (dayCell.isSelected)
                                        return control.selectedTextColor;
                                    if (dayCell.isToday)
                                        return control.todayColor;
                                    if (!dayCell.isCurrentMonth)
                                        return control.otherMonthDayColor;
                                    return control.normalDayColor;
                                }
                                bold: dayCell.isSelected || dayCell.isToday
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    control.focus = true;
                                    control.selectDate(dayCell.cellDate);
                                }
                            }
                        }
                    }
                }
            }
        }

        Text {
            visible: control.showFooter
            anchors.horizontalCenter: parent.horizontalCenter
            text: "────────────────────────────"
            color: control.gridBorderColor
        }

        Row {
            visible: control.showFooter
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 1

            Text {
                text: "Sel: " + control.formatIsoDate(control.selectedDate)
                color: "yellow"
            }
        }
    }
}
