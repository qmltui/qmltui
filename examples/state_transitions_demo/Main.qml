import QtQml 2.15
import Tui 1.0

Terminal {
    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Escape || (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_Q) || event.key === Qt.Key_Q) {
            Qt.quit()
            event.accepted = true
        }
    }

    id: rootWindow
    width: 80
    height: 24
    backgroundColor: "black"
    useAlternateScreen: true

    property bool sidebarActive: false

    Rectangle {
        anchors.fill: parent
        color: "#151520"
        border.style: "single"
        border.color: "#444455"

        // Header
        Rectangle {
            id: header
            width: parent.width
            height: 3
            color: "#222233"

            Text {
                anchors.fill: parent
                text: "INTERACTIVE STATES & TRANSITIONS DEMO"
                color: "cyan"
                bold: true
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
            }
        }

        // Main content container (clipped to hide off-screen sidebar)
        Item {
            id: contentContainer
            x: 1
            y: 3
            width: parent.width - 2
            height: parent.height - 5
            clip: true

            // Instructions and status
            Column {
                anchors.centerIn: parent
                spacing: 1

                Text {
                    text: "Sidebar State: " + (rootWindow.sidebarActive ? "EXPANDED" : "COLLAPSED")
                    color: rootWindow.sidebarActive ? "green" : "red"
                    bold: true
                    horizontalAlignment: Qt.AlignHCenter
                }

                Text {
                    text: "Press [SPACE] on the button below to toggle the sidebar."
                    color: "silver"
                    horizontalAlignment: Qt.AlignHCenter
                }

                Text {
                    text: "The sidebar will slide in from the left and change background color."
                    color: "gray"
                    horizontalAlignment: Qt.AlignHCenter
                }

                // Spacing
                Text { text: ""; height: 1 }

                // Interactive Toggle Button
                Rectangle {
                    id: toggleButton
                    width: 32
                    height: 3
                    focusable: true
                    focus: true
                    color: activeFocus ? "blue" : "#2a2a3e"

                    Text {
                        anchors.fill: parent
                        text: toggleButton.activeFocus ? ">>  TOGGLE SIDEBAR MENU  <<" : "    TOGGLE SIDEBAR MENU    "
                        color: toggleButton.activeFocus ? "white" : "cyan"
                        bold: toggleButton.activeFocus
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                    }

                    Keys.onPressed: (event) => {
                        if (event.key === Qt.Key_Space) {
                            rootWindow.sidebarActive = !rootWindow.sidebarActive;
                            event.accepted = true;
                        }
                    }
                }
            }

            // Collapsible Sidebar Menu
            Rectangle {
                id: sidebar
                x: -24
                y: 0
                width: 24
                height: parent.height
                color: "#222233"
                border.style: "double"
                border.color: "blue"

                // Sidebar Header
                Text {
                    text: "SIDEBAR MENU"
                    color: "yellow"
                    bold: true
                    x: 2
                    y: 1
                }

                // Sidebar Items
                Column {
                    x: 2
                    y: 3
                    spacing: 1

                    Text { text: "1. [D]ashboard"; color: "white" }
                    Text { text: "2. [P]rofile"; color: "white" }
                    Text { text: "3. [S]ettings"; color: "white" }
                    Text { text: "4. [H]elp"; color: "white" }
                    Text { text: ""; height: 1 }
                    Text { text: "[Esc] Close"; color: "gray" }
                }

                states: [
                    State {
                        name: "expanded"
                        when: rootWindow.sidebarActive
                        PropertyChanges {
                            target: sidebar
                            properties: {
                                "x": 0,
                                "color": "#444466"
                            }
                        }
                    },
                    State {
                        name: "collapsed"
                        when: !rootWindow.sidebarActive
                        PropertyChanges {
                            target: sidebar
                            properties: {
                                "x": -24,
                                "color": "#222233"
                            }
                        }
                    }
                ]

                transitions: [
                    Transition {
                        from: "collapsed"; to: "expanded"
                        NumberAnimation {
                            duration: 500
                            easing: Easing.OutQuad
                        }
                        ColorAnimation {
                            duration: 500
                        }
                    },
                    Transition {
                        from: "expanded"; to: "collapsed"
                        NumberAnimation {
                            duration: 400
                            easing: Easing.InQuad
                        }
                        ColorAnimation {
                            duration: 400
                        }
                    }
                ]
            }

            // Keyboard listener for ESC key to close sidebar from anywhere
            Item {
                focusable: true
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Escape && rootWindow.sidebarActive) {
                        rootWindow.sidebarActive = false;
                        event.accepted = true;
                    }
                }
            }
        }

        // Footer status bar
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 1
            width: parent.width
            height: 1
            color: "#181822"

            Text {
                anchors.fill: parent
                text: " Press [SPACE] to toggle | [Esc] to close sidebar | [Ctrl+C] to exit"
                color: "gray"
            }
        }
    }
}
