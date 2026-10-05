pragma ComponentBehavior: Bound

import QtQml
import Qtui
import Qtui.Controls

Rectangle {
    id: root

    required property string userHost
    required property string breadcrumbs
    required property string diskUsage
    required property string modeName
    required property string modeColor

    height: 1
    color: Theme.bgHeader
    border.style: "none"

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 1
        spacing: 1

        // User / Host pill
        Rectangle {
            width: userText.implicitWidth + 2
            height: 1
            color: Theme.accentMauve

            Text {
                id: userText
                anchors.centerIn: parent
                text: " " + root.userHost + " "
                color: "#11111b"
                bold: true
            }
        }

        // Path Breadcrumbs
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.breadcrumbs
            color: Theme.accentBlue
            bold: true
        }
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 1
        spacing: 1

        // Mode Pill
        Rectangle {
            width: modeText.implicitWidth + 2
            height: 1
            color: root.modeColor

            Text {
                id: modeText
                anchors.centerIn: parent
                text: " " + root.modeName + " "
                color: "#11111b"
                bold: true
            }
        }

        // Disk Usage
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: " " + root.diskUsage + " "
            color: Theme.textSecondary
        }
    }
}
