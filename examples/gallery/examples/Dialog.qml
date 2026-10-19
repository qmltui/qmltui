pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property string statusText: "No decision yet"
    property int decisionCount: 0
    property string lastAudit: "Awaiting confirmation"
    property bool showReject: true

    function recordDecision(outcome, tag) {
        statusText = outcome;
        decisionCount += 1;
        lastAudit = "#" + decisionCount + " " + tag;
        deleteButton.focus = true;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            title: Row {
                spacing: 1
                Text {
                    text: "Modal Dialog Stage"
                    color: "#f87171"
                }
                Text {
                    text: "[Controls]"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: confirmDialog.visible ? "OPEN" : "IDLE"
                color: confirmDialog.visible ? "#2cde85" : "#8b949e"
            }
            footer: Text {
                text: "Enter open/accept · Esc reject"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "Modal"
                color: "#8b949e"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Target Artifact: /var/reports/report.pdf"
                    color: "#c9d1d9"
                    bold: true
                }

                Text {
                    text: "Status: " + root.statusText
                    color: root.statusText === "Deleted report.pdf" ? "#f87171" : (root.statusText === "Kept report.pdf" ? "#2cde85" : "#fbbf24")
                    bold: true
                }

                Text {
                    text: "Policy: CloseOnEscape | Modal Dimming"
                    color: "#8b949e"
                }

                Button {
                    id: deleteButton

                    focus: true
                    text: "Delete report.pdf…"
                    textColor: "#f87171"
                    onClicked: confirmDialog.open()
                }
            }
        }

        Panel {
            Layout.preferredWidth: 28
            Layout.fillHeight: true
            color: "#161b22"
            border {
                style: Border.Single
                color: "#30363d"
            }

            title: Text {
                text: "Inspector"
                color: "#58a6ff"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "dialog.visible"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + (confirmDialog.visible ? "true" : "false")
                    color: confirmDialog.visible ? "#2cde85" : "#f0f6fc"
                    bold: true
                }

                CheckBox {
                    id: rejectCheck

                    text: "showRejectButton"
                    checked: root.showReject
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.showReject = checked;
                    }
                }

                Text {
                    text: "Decisions: " + root.decisionCount
                    color: "#58a6ff"
                    bold: true
                }

                Text {
                    text: "Audit Log:"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.lastAudit
                    color: "#c9d1d9"
                }
            }
        }
    }

    Dialog {
        id: confirmDialog

        title: "Confirm Removal"
        titleColor: "#f87171"
        textColor: "#c9d1d9"
        message: "report.pdf will be removed permanently."
        acceptText: "Delete"
        rejectText: "Cancel"
        showRejectButton: root.showReject
        onAccepted: root.recordDecision("Deleted report.pdf", "DELETED")
        onRejected: root.recordDecision("Kept report.pdf", "KEPT")
    }
}
