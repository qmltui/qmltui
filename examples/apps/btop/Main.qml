pragma ComponentBehavior: Bound

import QtQml
import Qtui
import Qtui.Controls

Terminal {
    id: root
    color: backgroundColor

    readonly property string backgroundColor: "#080a0b"
    readonly property string panelColor: "#0b0e0f"
    readonly property string borderColor: "#56604a"
    readonly property string textColor: "#d8dee9"
    readonly property string mutedColor: "#737d67"
    readonly property string greenColor: "#a6e22e"
    readonly property string cyanColor: "#66d9ef"
    readonly property string yellowColor: "#e6db74"
    readonly property string pinkColor: "#f92672"
    readonly property string purpleColor: "#ae81ff"
    property string clockText: Qt.formatTime(new Date(), "HH:mm:ss")
    property bool cpuPanelVisible: true
    property bool memoryPanelVisible: true
    property bool networkPanelVisible: true
    property bool processPanelVisible: true
    readonly property bool sidePanelsVisible: memoryPanelVisible || networkPanelVisible
    readonly property bool upperPanelsVisible: processPanelVisible || sidePanelsVisible
    readonly property bool cpuFocusMode: cpuPanelVisible && !upperPanelsVisible

    BtopModel {
        id: systemModel
    }

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.clockText = Qt.formatTime(new Date(), "HH:mm:ss")
    }

    function handleKeyEvent(event) {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_Space) {
            systemModel.togglePaused()
            event.accepted = true
        } else if (event.key === Qt.Key_C) {
            systemModel.sortByCpu()
            event.accepted = true
        } else if (event.key === Qt.Key_M) {
            systemModel.sortByMemory()
            event.accepted = true
        } else if (event.key === Qt.Key_P) {
            systemModel.sortByPid()
            event.accepted = true
        } else if (event.key === Qt.Key_R) {
            systemModel.cycleRefreshInterval()
            event.accepted = true
        } else if (event.key === Qt.Key_1) {
            root.cpuPanelVisible = !root.cpuPanelVisible
            event.accepted = true
        } else if (event.key === Qt.Key_2) {
            root.memoryPanelVisible = !root.memoryPanelVisible
            event.accepted = true
        } else if (event.key === Qt.Key_3) {
            root.networkPanelVisible = !root.networkPanelVisible
            event.accepted = true
        } else if (event.key === Qt.Key_4) {
            root.processPanelVisible = !root.processPanelVisible
            event.accepted = true
        }
    }

    Keys.onPressed: (event) => root.handleKeyEvent(event)

    Rectangle {
        id: header

        visible: !root.cpuFocusMode
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: visible ? 1 : 0
        color: root.backgroundColor

        Row {
            anchors.left: parent.left

            Text {
                text: qsTr(" 1cpu")
                color: root.cpuPanelVisible ? root.greenColor : root.mutedColor
                bold: root.cpuPanelVisible
            }

            Text {
                text: qsTr("  2mem")
                color: root.memoryPanelVisible ? root.pinkColor : root.mutedColor
                bold: root.memoryPanelVisible
            }

            Text {
                text: qsTr("  3net")
                color: root.networkPanelVisible ? root.purpleColor : root.mutedColor
                bold: root.networkPanelVisible
            }

            Text {
                text: qsTr("  4proc")
                color: root.processPanelVisible ? root.yellowColor : root.mutedColor
                bold: root.processPanelVisible
            }
        }

        Text {
            anchors.centerIn: parent
            text: systemModel.paused
                  ? qsTr("btop  PAUSED")
                  : qsTr("btop  %1@%2").arg(systemModel.userName).arg(systemModel.hostName)
            color: systemModel.paused ? root.pinkColor : root.textColor
            bold: true
        }

        Text {
            anchors.right: parent.right
            text: qsTr("%1  %2ms ").arg(root.clockText).arg(systemModel.refreshInterval)
            color: root.cyanColor
        }
    }

    Item {
        id: dashboard

        anchors.top: header.visible ? header.bottom : parent.top
        anchors.bottom: footer.visible ? footer.top : parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        Item {
            id: upperRegion

            clip: true
            visible: height > 0
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: !root.upperPanelsVisible ? 0
                    : root.cpuPanelVisible
                      ? Math.max(19, Math.floor(parent.height * 0.64))
                      : parent.height

            Behavior on height {
                NumberAnimation {
                    duration: 150
                    easing: Easing.InOutQuad
                }
            }

            Panel {
                id: processPanel

                clip: true
                visible: width > 0
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                width: !root.processPanelVisible ? 0
                       : root.sidePanelsVisible ? Math.floor(parent.width * 0.55)
                       : parent.width
                color: root.panelColor
                border.style: "rounded"
                border.color: root.borderColor

                title: qsTr("4proc  PROCESSES")
                titleColor: root.pinkColor
                titleTrailing: qsTr("sort: %1  total: %2").arg(systemModel.sortMode)
                                  .arg(systemModel.totalProcesses)
                titleTrailingColor: root.yellowColor
                footer: qsTr("↑↓ select  Enter inspect")
                footerColor: root.mutedColor
                footerTrailing: qsTr("%1/%2").arg(Math.max(0, processTable.selectedRow + 1))
                                      .arg(systemModel.totalProcesses)
                footerTrailingColor: root.textColor

                Behavior on width {
                    NumberAnimation {
                        duration: 150
                        easing: Easing.InOutQuad
                    }
                }

                Item {
                    id: processSummary

                    anchors.top: parent.top
                    anchors.topMargin: 1
                    anchors.left: parent.left
                    anchors.leftMargin: 1
                    anchors.right: parent.right
                    anchors.rightMargin: 1
                    height: 5

                    Text {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        text: qsTr("CPU %1%  load %2")
                                  .arg(systemModel.cpuUsage.toFixed(1))
                                  .arg(systemModel.loadAverageText)
                        color: root.textColor
                    }

                    Text {
                        anchors.top: parent.top
                        anchors.right: parent.right
                        text: qsTr("up %1").arg(systemModel.uptimeText)
                        color: root.mutedColor
                    }

                    Sparkline {
                        anchors.top: parent.top
                        anchors.topMargin: 1
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 2
                        values: systemModel.cpuHistory
                        from: 0
                        to: 100
                        filled: true
                        color: systemModel.cpuUsage > 80 ? root.pinkColor : root.greenColor
                        backgroundColor: root.panelColor
                    }

                    Text {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        text: qsTr("program activity")
                        color: root.mutedColor
                    }

                    Text {
                        anchors.bottom: parent.bottom
                        anchors.right: parent.right
                        text: qsTr("[c]cpu [m]mem [p]pid")
                        color: root.cyanColor
                    }
                }

                TableView {
                    id: processTable

                    anchors.top: processSummary.bottom
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 1
                    anchors.left: parent.left
                    anchors.leftMargin: 1
                    anchors.right: parent.right
                    anchors.rightMargin: 1
                    focus: true
                    Keys.onPressed: (event) => root.handleKeyEvent(event)
                    model: systemModel
                    columns: systemModel.columnDefinitions()
                    columnWidthProvider: (column) => {
                        if (column === 0) return 8   // PID
                        if (column === 1) return 15  // Program
                        if (column === 3) return 6   // Thr
                        if (column === 4) return 9   // User
                        if (column === 5) return 10  // Mem
                        if (column === 6) return 7   // Cpu%
                        if (column === 2) {
                            const fixedWidths = 8 + 15 + 6 + 9 + 10 + 7
                            return Math.max(16, processTable.width - fixedWidths)
                        }
                        return 10
                    }
                    headerColor: root.textColor
                    headerBackgroundColor: root.panelColor
                    selectionColor: root.textColor
                    selectionBackgroundColor: "#344318"
                    textColor: root.textColor
                    rowBackgroundColor: root.panelColor
                    alternatingRowBackgroundColor: "#101411"
                    alternatingRows: true
                    showColumnGrid: false
                    cellFormat: (row, column, value) => {
                        if (column === 1)
                            return { text: value, color: root.greenColor }
                        if (column === 5)
                            return { text: value, color: root.yellowColor }
                        if (column === 6) {
                            const usage = parseFloat(value)
                            return { text: value,
                                     color: usage > 80 ? root.pinkColor : root.greenColor,
                                     bold: usage > 20 }
                        }
                        return { text: value, color: root.textColor }
                    }
                }
            }

            Item {
                id: sideRegion

                clip: true
                visible: (root.sidePanelsVisible || processPanel.width < parent.width)
                         && (parent.width - processPanel.width > 0)
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: processPanel.right
                anchors.right: parent.right

                Panel {
                    id: networkPanel

                    clip: true
                    visible: height > 0
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: !root.networkPanelVisible ? 0
                            : root.memoryPanelVisible
                              ? Math.max(8, Math.floor(parent.height * 0.36))
                              : parent.height
                    color: root.panelColor
                    border.style: "rounded"
                    border.color: root.borderColor

                    title: qsTr("3net  NETWORK")
                    titleColor: root.purpleColor
                    titleTrailing: systemModel.networkInterface
                    titleTrailingColor: root.textColor

                    Behavior on height {
                        NumberAnimation {
                            duration: 150
                            easing: Easing.InOutQuad
                        }
                    }

                    Text {
                        anchors.top: parent.top
                        anchors.topMargin: 1
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        text: qsTr("▼ download  %1").arg(systemModel.downloadRateText)
                        color: root.cyanColor
                    }

                    Sparkline {
                        anchors.top: parent.top
                        anchors.topMargin: 2
                        anchors.bottom: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 1
                        anchors.right: parent.right
                        anchors.rightMargin: 1
                        values: systemModel.downloadHistory
                        filled: true
                        color: root.purpleColor
                        backgroundColor: root.panelColor
                    }

                    Text {
                        anchors.top: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        text: qsTr("▲ upload    %1").arg(systemModel.uploadRateText)
                        color: root.pinkColor
                    }

                    Sparkline {
                        anchors.top: parent.verticalCenter
                        anchors.topMargin: 1
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        anchors.left: parent.left
                        anchors.leftMargin: 1
                        anchors.right: parent.right
                        anchors.rightMargin: 1
                        values: systemModel.uploadHistory
                        filled: true
                        color: root.pinkColor
                        backgroundColor: root.panelColor
                    }
                }

                Panel {
                    id: memoryPanel

                    clip: true
                    visible: height > 0 && (root.memoryPanelVisible
                             || (root.networkPanelVisible && networkPanel.height < parent.height))
                    anchors.top: networkPanel.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: Math.floor(parent.width * 0.55)
                    color: root.panelColor
                    border.style: "rounded"
                    border.color: root.borderColor

                    title: qsTr("2mem  MEMORY")
                    titleColor: root.pinkColor

                    Column {
                        anchors.top: parent.top
                        anchors.topMargin: 1
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        anchors.right: parent.right
                        anchors.rightMargin: 2
                        spacing: 0

                        Text {
                            width: parent.width
                            text: qsTr("Total %1").arg(systemModel.memoryTotalText)
                            color: root.textColor
                            bold: true
                        }

                        Text {
                            width: parent.width
                            text: qsTr("Used %1  %2%")
                                      .arg(systemModel.memoryUsedText)
                                      .arg(systemModel.memoryPercent.toFixed(0))
                            color: root.pinkColor
                        }

                        ProgressBar {
                            width: parent.width
                            value: systemModel.memoryPercent
                            from: 0
                            to: 100
                            color: root.pinkColor
                        }

                        Text {
                            width: parent.width
                            text: qsTr("Available %1").arg(systemModel.memoryAvailableText)
                            color: root.yellowColor
                        }

                        ProgressBar {
                            width: parent.width
                            value: systemModel.availablePercent
                            from: 0
                            to: 100
                            color: root.yellowColor
                        }

                        Text {
                            width: parent.width
                            text: qsTr("Cached %1").arg(systemModel.memoryCachedText)
                            color: root.cyanColor
                        }

                        ProgressBar {
                            width: parent.width
                            value: systemModel.cachedPercent
                            from: 0
                            to: 100
                            color: root.cyanColor
                        }
                    }
                }

                Panel {
                    clip: true
                    visible: height > 0 && (root.memoryPanelVisible
                             || (root.networkPanelVisible && networkPanel.height < parent.height))
                    anchors.top: networkPanel.bottom
                    anchors.bottom: parent.bottom
                    anchors.left: memoryPanel.right
                    anchors.right: parent.right
                    color: root.panelColor
                    border.style: "rounded"
                    border.color: root.borderColor

                    title: qsTr("disks")
                    titleColor: root.purpleColor

                    Column {
                        anchors.top: parent.top
                        anchors.topMargin: 1
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 1
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        anchors.right: parent.right
                        anchors.rightMargin: 2
                        spacing: 0

                        Text {
                            width: parent.width
                            text: qsTr("root /  %1").arg(systemModel.diskTotalText)
                            color: root.textColor
                            bold: true
                        }

                        Text {
                            width: parent.width
                            text: qsTr("Used %1  %2%")
                                      .arg(systemModel.diskUsedText)
                                      .arg(systemModel.diskPercent.toFixed(0))
                            color: root.pinkColor
                        }

                        ProgressBar {
                            width: parent.width
                            value: systemModel.diskPercent
                            from: 0
                            to: 100
                            color: root.pinkColor
                        }

                        Text {
                            width: parent.width
                            text: qsTr("swap %1").arg(systemModel.swapTotalText)
                            color: root.textColor
                            bold: true
                        }

                        Text {
                            width: parent.width
                            text: qsTr("Used %1  %2%")
                                      .arg(systemModel.swapUsedText)
                                      .arg(systemModel.swapPercent.toFixed(0))
                            color: root.greenColor
                        }

                        ProgressBar {
                            width: parent.width
                            value: systemModel.swapPercent
                            from: 0
                            to: 100
                            color: root.greenColor
                        }

                        Text {
                            width: parent.width
                            text: systemModel.osName
                            color: root.mutedColor
                        }
                    }
                }
            }
        }

        Rectangle {
            id: cpuPanel

            clip: true
            visible: height > 0 && (root.cpuPanelVisible
                     || (root.upperPanelsVisible && upperRegion.height < parent.height))
            anchors.top: upperRegion.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            color: root.panelColor
            border.style: "rounded"
            border.color: root.borderColor

            Text {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.leftMargin: 2
                text: qsTr("┐¹cpu┌──┐menu┌┐preset *┌")
                color: root.greenColor
                bold: true
            }

            Text {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("┐%1┌").arg(root.clockText)
                color: root.textColor
            }

            Text {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.rightMargin: 2
                text: qsTr("┐- %1ms +┌").arg(systemModel.refreshInterval)
                color: root.cyanColor
            }

            Item {
                id: cpuGraphArea

                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 1
                anchors.left: parent.left
                anchors.leftMargin: 1
                anchors.right: corePanel.left

                BtopCpuGraph {
                    id: cpuBrailleCanvas

                    anchors.fill: parent
                    values: systemModel.cpuHistory
                    color: systemModel.cpuUsage > 80 ? root.pinkColor : root.greenColor
                    backgroundColor: root.panelColor
                }

                Text {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    text: qsTr("up %1").arg(systemModel.uptimeText)
                    color: root.mutedColor
                }
            }

            Rectangle {
                id: corePanel

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 1
                width: 25
                height: Math.min(parent.height - 2,
                                 Math.max(6, Math.min(systemModel.coreUsage.length, 16) + 4))
                color: root.panelColor
                border.style: "rounded"
                border.color: root.borderColor

                Text {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.leftMargin: 2
                    text: qsTr("┐%1┌┐%2┌")
                          .arg(systemModel.cpuNameText.slice(
                                   0, Math.max(4, corePanel.width - systemModel.cpuFrequencyText.length - 7)))
                          .arg(systemModel.cpuFrequencyText)
                    color: root.cyanColor
                    bold: true
                }

                Row {
                    anchors.top: parent.top
                    anchors.topMargin: 1
                    anchors.left: parent.left
                    anchors.leftMargin: 2
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    spacing: 1

                    Text {
                        width: 4
                        text: qsTr("CPU")
                        color: root.textColor
                        bold: true
                    }

                    ProgressBar {
                        width: Math.max(5, parent.width - 11)
                        value: systemModel.cpuUsage
                        from: 0
                        to: 100
                        fillCharacter: "■"
                        emptyCharacter: " "
                        color: systemModel.cpuUsage > 80 ? root.pinkColor : root.greenColor
                    }

                    Text {
                        width: 5
                        text: qsTr("%1%").arg(systemModel.cpuUsage.toFixed(0).padStart(3))
                        color: systemModel.cpuUsage > 80 ? root.pinkColor : root.textColor
                    }
                }

                Column {
                    id: coreColumn

                    anchors.top: parent.top
                    anchors.topMargin: 2
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 2
                    anchors.left: parent.left
                    anchors.leftMargin: 2
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    spacing: 0

                    Repeater {
                        model: Math.min(systemModel.coreUsage.length,
                                        Math.max(1, coreColumn.height))

                        Row {
                            id: coreRow

                            required property int index
                            width: coreColumn.width
                            spacing: 1

                            Text {
                                width: 3
                                text: qsTr("C%1").arg(coreRow.index).padEnd(3)
                                color: root.textColor
                            }

                            Text {
                                id: coreHistoryText

                                width: Math.max(6, coreColumn.width - 10)
                                text: systemModel.coreHistoryText[coreRow.index] || ""
                                color: Number(systemModel.coreUsage[coreRow.index]) > 80
                                       ? root.pinkColor : root.greenColor
                            }

                            Text {
                                width: 5
                                text: qsTr("%1%").arg(
                                          Number(systemModel.coreUsage[coreRow.index]).toFixed(0).padStart(3))
                                color: Number(systemModel.coreUsage[coreRow.index]) > 80
                                       ? root.pinkColor : root.textColor
                            }
                        }
                    }
                }

                Text {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.leftMargin: 2
                    text: qsTr("LAV: %1").arg(systemModel.loadAverageText)
                    color: root.textColor
                }
            }
        }
    }

    Rectangle {
        id: footer

        visible: !root.cpuFocusMode
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: visible ? 1 : 0
        color: root.backgroundColor

        Text {
            anchors.left: parent.left
            text: qsTr(" [1-4] panels  [space] pause  [c/m/p] sort  [r] interval")
            color: root.mutedColor
        }

        Text {
            anchors.right: parent.right
            text: qsTr("[q/esc] quit ")
            color: root.pinkColor
            bold: true
        }
    }
}
