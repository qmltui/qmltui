pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property string lastToggled: "None"
    property var checklistItems: [
        {
            display: "Verify TLS certificates",
            decoration: "◆",
            checkable: true,
            checkState: Qt.Checked,
            foreground: "#2cde85",
            enabled: true
        },
        {
            display: "Run database migrations",
            decoration: "◆",
            checkable: true,
            checkState: Qt.Checked,
            foreground: "#2cde85",
            enabled: true
        },
        {
            display: "Warm edge CDN caches",
            decoration: "◇",
            checkable: true,
            checkState: Qt.Unchecked,
            foreground: "#7ee0ff",
            enabled: true
        },
        {
            display: "Drain canary traffic",
            decoration: "◇",
            checkable: true,
            checkState: Qt.Unchecked,
            foreground: "#f9c74f",
            enabled: true
        },
        {
            display: "Promote release tag",
            decoration: "◇",
            checkable: true,
            checkState: Qt.Unchecked,
            foreground: "#c084fc",
            enabled: true
        },
        {
            display: "Legacy v1 endpoint (locked)",
            decoration: "•",
            checkable: true,
            checkState: Qt.PartiallyChecked,
            foreground: "#8b949e",
            enabled: false
        }
    ]
    readonly property int checkedCount: {
        let count = 0;
        for (let i = 0; i < checklistItems.length; ++i) {
            if (checklistItems[i].checkState === Qt.Checked)
                count += 1;
        }
        return count;
    }
    readonly property string currentItemName: {
        const idx = checklistView.currentIndex;
        if (idx >= 0 && idx < checklistItems.length)
            return checklistItems[idx].display;
        return "None";
    }

    function toggleRow(index) {
        if (index < 0 || index >= checklistItems.length)
            return;
        const entry = checklistItems[index];
        if (entry.enabled === false || !entry.checkable)
            return;
        const nextState = entry.checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked;
        let updated = [];
        for (let i = 0; i < checklistItems.length; ++i) {
            if (i === index) {
                updated.push({
                    display: entry.display,
                    decoration: nextState === Qt.Checked ? "◆" : "◇",
                    checkable: entry.checkable,
                    checkState: nextState,
                    foreground: nextState === Qt.Checked ? "#2cde85" : "#7ee0ff",
                    enabled: entry.enabled
                });
            } else {
                updated.push(checklistItems[i]);
            }
        }
        const savedIndex = checklistView.currentIndex;
        checklistItems = updated;
        checklistView.currentIndex = savedIndex;
        lastToggled = entry.display;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "StandardItemDelegate"
            titleColor: "#93c5fd"
            titleBadge: "Controls"
            titleBadgeColor: "#58a6ff"
            titleTrailing: root.checkedCount + " / 5"
            titleTrailingColor: "#2cde85"
            footer: "Up/Down move  Space toggle"
            footerColor: "#8b949e"
            footerTrailing: "row " + checklistView.currentIndex
            footerTrailingColor: "#93c5fd"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    leftMargin: 2
                    rightMargin: 2
                    topMargin: 1
                    bottomMargin: 1
                }
                spacing: 1

                Text {
                    text: "Release Deployment Checklist"
                    color: "#c9d1d9"
                    bold: true
                }

                Rectangle {
                    width: parent.width
                    height: 8
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    ListView {
                        id: checklistView

                        anchors {
                            fill: parent
                            margins: 1
                        }
                        focus: true
                        currentIndex: 0
                        model: root.checklistItems
                        delegate: StandardItemDelegate {
                            textColor: "#c9d1d9"
                        }

                        onItemActivated: index => {
                            root.toggleRow(index);
                        }
                    }
                }

                Text {
                    text: "Selected: " + root.currentItemName
                    color: "#93c5fd"
                }

                Text {
                    text: "Last toggled: " + root.lastToggled
                    color: "#2cde85"
                }
            }
        }

        Panel {
            Layout.preferredWidth: 28
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    leftMargin: 2
                    rightMargin: 2
                    topMargin: 1
                    bottomMargin: 1
                }
                spacing: 1

                Button {
                    text: "Toggle Row"
                    textColor: "#2cde85"

                    onClicked: {
                        root.toggleRow(checklistView.currentIndex);
                    }
                }

                Button {
                    text: "Next Row"
                    textColor: "#93c5fd"

                    onClicked: {
                        checklistView.currentIndex = (checklistView.currentIndex + 1) % root.checklistItems.length;
                    }
                }

                Column {
                    spacing: 0

                    Text {
                        text: "TELEMETRY"
                        color: "#58a6ff"
                        bold: true
                    }

                    Text {
                        text: "row: " + checklistView.currentIndex
                        color: "#93c5fd"
                        bold: true
                    }

                    Text {
                        text: "checked: " + root.checkedCount + " / 5"
                        color: "#2cde85"
                    }

                    Text {
                        text: "roles: display,check"
                        color: "#8b949e"
                    }

                    Text {
                        text: "last:"
                        color: "#8b949e"
                    }

                    Text {
                        text: root.lastToggled
                        color: "#f9c74f"
                    }
                }
            }
        }
    }
}
