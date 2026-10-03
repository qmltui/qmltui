import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        }
    }

    readonly property var mockEvents: ({
        "2026-08-18": ["Qtui v2.0 Release Meeting (10:00 AM)", "Code Review: Calendar Widget (02:00 PM)", "Team Sync (05:00 PM)"],
        "2026-08-20": ["Sprint Planning (09:30 AM)", "Demo Prep (03:00 PM)"],
        "2026-08-25": ["Architecture Review (11:00 AM)"],
        "2026-09-01": ["Monthly Goals Sync (09:00 AM)"]
    })

    Column {
        anchors.centerIn: parent
        spacing: 1

        // Title Banner
        Panel {
            title: "Qtui Reusable Calendar Component Example"
            titleAlignment: Qt.AlignHCenter
            titleColor: "cyan"
            width: 76
            height: 3

            Text {
                anchors.centerIn: parent
                text: "Interactive Terminal Calendar Widget with Keyboard & Mouse Navigation"
                color: "white"
            }
        }

        // Main Content Area (Calendar + Sidebar)
        Row {
            spacing: 2

            // Reusable Calendar Control Instance
            Calendar {
                id: mainCalendar
                focus: true
                showFooter: true
                firstDayOfWeek: 0

                onDateSelected: (dateVal) => {
                    var dateStr = dateVal ? dateVal.toISOString().slice(0, 10) : ""
                    statusText.text = "Selected Date: " + dateStr
                }
            }

            // Right Sidebar (Agenda + Settings + Help)
            Column {
                spacing: 1

                // Selected Date Agenda Panel
                Panel {
                    title: "Agenda & Events"
                    titleColor: "yellow"
                    width: 42
                    height: 7

                    Column {
                        x: 2
                        y: 1
                        spacing: 0

                        Text {
                            property string dateStr: mainCalendar.selectedDate ? mainCalendar.selectedDate.toISOString().slice(0, 10) : ""
                            text: "Date: " + dateStr
                            color: "cyan"
                            bold: true
                        }

                        Text {
                            text: "──────────────────────────────────────"
                            color: "gray"
                        }

                        Repeater {
                            model: {
                                var dateStr = mainCalendar.selectedDate ? mainCalendar.selectedDate.toISOString().slice(0, 10) : ""
                                var list = root.mockEvents[dateStr]
                                return list ? list : ["No events scheduled for this day."]
                            }
                            delegate: Text {
                                required property string modelData
                                text: "• " + modelData
                                color: modelData.startsWith("No events") ? "gray" : "green"
                            }
                        }
                    }
                }

                // Widget Settings / Action Panel
                Panel {
                    title: "Options & Controls"
                    titleColor: "magenta"
                    width: 42
                    height: 6

                    Column {
                        x: 2
                        y: 1
                        spacing: 1

                        Row {
                            spacing: 2

                            Button {
                                label: "[ Today ]"
                                onClicked: mainCalendar.goToToday()
                            }

                            Button {
                                label: mainCalendar.firstDayOfWeek === 0 ? "[ Start: Mon ]" : "[ Start: Sun ]"
                                onClicked: mainCalendar.firstDayOfWeek = (mainCalendar.firstDayOfWeek === 0 ? 1 : 0)
                            }

                            Button {
                                label: mainCalendar.showFooter ? "[ Hide Footer ]" : "[ Show Footer ]"
                                onClicked: mainCalendar.showFooter = !mainCalendar.showFooter
                            }
                        }

                        Row {
                            spacing: 2

                            Button {
                                label: "[ < Prev Month ]"
                                onClicked: mainCalendar.prevMonth()
                            }

                            Button {
                                label: "[ Next Month > ]"
                                onClicked: mainCalendar.nextMonth()
                            }
                        }
                    }
                }
            }
        }

        // Navigation Help Panel
        Panel {
            title: "Keyboard & Mouse Controls"
            titleColor: "green"
            width: 76
            height: 4

            Row {
                x: 2
                y: 1
                spacing: 4

                Column {
                    Text { text: "• Arrow Keys / H,J,K,L: Move Day / Week"; color: "white" }
                    Text { text: "• PgUp / PgDn / [ / ]: Prev / Next Month"; color: "white" }
                }

                Column {
                    Text { text: "• Home / T: Jump to Today"; color: "white" }
                    Text { text: "• Enter / Space: Select Date"; color: "white" }
                }

                Column {
                    Text { text: "• Mouse Click: Direct Selection"; color: "white" }
                    Text { text: "• Q / Esc: Quit Example"; color: "yellow" }
                }
            }
        }

        // Status Bar
        Text {
            id: statusText
            text: "Status: Ready. Use arrow keys to navigate dates."
            color: "cyan"
        }
    }
}
