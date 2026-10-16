pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property string lastActivated: "none"
    readonly property var jobs: [
        {
            jobName: "fetch-sources",
            status: "DONE",
            duration: "0.8s",
            stage: "Prepare"
        },
        {
            jobName: "configure-cmake",
            status: "DONE",
            duration: "2.4s",
            stage: "Configure"
        },
        {
            jobName: "compile-core",
            status: "RUNNING",
            duration: "6.1s",
            stage: "Build"
        },
        {
            jobName: "compile-controls",
            status: "RUNNING",
            duration: "3.9s",
            stage: "Build"
        },
        {
            jobName: "lint-qml-strict",
            status: "READY",
            duration: "1.1s",
            stage: "Verify"
        },
        {
            jobName: "ctest-runner",
            status: "READY",
            duration: "4.5s",
            stage: "Test"
        },
        {
            jobName: "pack-wasm-demo",
            status: "READY",
            duration: "2.0s",
            stage: "Package"
        },
        {
            jobName: "publish-docs",
            status: "READY",
            duration: "1.7s",
            stage: "Deploy"
        }
    ]
    readonly property var activeJob: listView.currentIndex >= 0 && listView.currentIndex < jobs.length ? jobs[listView.currentIndex] : jobs[0]

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_S) {
                listView.spacing = listView.spacing === 0 ? 1 : 0;
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "ListView · Deployment Pipeline"
            titleColor: "#f8fafc"
            titleBadge: "[CORE]"
            titleBadgeColor: "#f472b6"
            titleTrailing: (listView.currentIndex + 1) + "/" + root.jobs.length
            titleTrailingColor: "#8b949e"
            footer: "↑↓ select · Enter run · S spacing"
            footerColor: "#8b949e"
            footerTrailing: "spacing:" + listView.spacing
            footerTrailingColor: "#58a6ff"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            ListView {
                id: listView
                anchors {
                    fill: parent
                    margins: 1
                }
                focus: true
                currentIndex: 0
                spacing: 0
                reuseItems: true
                model: root.jobs

                onItemActivated: index => {
                    if (index >= 0 && index < root.jobs.length)
                        root.lastActivated = root.jobs[index].jobName;
                }

                delegate: Rectangle {
                    id: jobRow
                    required property int index
                    required property string jobName
                    required property string status
                    required property string duration
                    required property string stage
                    readonly property bool current: ListView.isCurrentItem
                    readonly property string badgeColor: status === "DONE" ? "#2cde85" : (status === "RUNNING" ? "#38bdf8" : "#fbbf24")
                    width: ListView.view ? ListView.view.width : 36
                    height: 1
                    color: current ? "#12261e" : (index % 2 === 0 ? "#0d1117" : "#161b22")

                    Text {
                        x: 0
                        y: 0
                        spans: [
                            {
                                text: jobRow.current ? "▸ " : "  ",
                                color: "#2cde85",
                                bold: true
                            },
                            {
                                text: jobRow.jobName,
                                color: jobRow.current ? "#f8fafc" : "#c9d1d9",
                                bold: jobRow.current
                            }
                        ]
                    }

                    Text {
                        anchors {
                            right: parent.right
                            rightMargin: 1
                        }
                        y: 0
                        spans: [
                            {
                                text: "[" + jobRow.status + "] ",
                                color: jobRow.badgeColor,
                                bold: true
                            },
                            {
                                text: jobRow.duration,
                                color: "#8b949e"
                            }
                        ]
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 26
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            footer: "ListView API"
            footerColor: "#8b949e"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 0

                Text {
                    text: "SELECTED JOB"
                    bold: true
                    color: "#f472b6"
                }

                Text {
                    text: "currentIndex: " + listView.currentIndex
                    color: "#c9d1d9"
                }

                Text {
                    width: parent.width
                    text: "Job:   " + root.activeJob.jobName
                    bold: true
                    color: "#f8fafc"
                }

                Text {
                    text: "State: " + root.activeJob.status
                    color: root.activeJob.status === "DONE" ? "#2cde85" : (root.activeJob.status === "RUNNING" ? "#38bdf8" : "#fbbf24")
                }

                Text {
                    text: "Stage: " + root.activeJob.stage
                    color: "#c9d1d9"
                }

                Text {
                    text: "Time:  " + root.activeJob.duration
                    color: "#8b949e"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "VIEW TELEMETRY"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "spacing: " + listView.spacing
                    color: "#c9d1d9"
                }

                Text {
                    width: parent.width
                    text: "Run: " + root.lastActivated
                    color: "#2cde85"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "OPTIONS"
                    bold: true
                    color: "#58a6ff"
                }

                CheckBox {
                    text: "Row spacing (S)"
                    checked: listView.spacing > 0
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        listView.spacing = checked ? 1 : 0;
                    }
                }

                CheckBox {
                    text: "reuseItems"
                    checked: listView.reuseItems
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        listView.reuseItems = checked;
                    }
                }
            }
        }
    }
}
