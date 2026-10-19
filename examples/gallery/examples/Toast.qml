pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property string toastPos: "top-right"
    property int totalSent: 0
    property string lastToast: "None"

    function dispatchToast(title, message, kind) {
        totalSent += 1;
        lastToast = "[" + kind + "] " + title;
        toastHost.showToast(title, message, kind, 2500);
    }

    function togglePosition() {
        toastPos = (toastPos === "top-right") ? "bottom-right" : "top-right";
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_P) {
                root.togglePosition();
                event.accepted = true;
            }
        }

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
                    text: "Toast Dispatcher"
                    color: "#34d399"
                }
                Text {
                    text: "[Controls]"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.toastPos
                color: "#8b949e"
            }
            footer: Text {
                text: "Enter dispatch · P toggle corner"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "Toast"
                color: "#8b949e"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 2
                }
                spacing: 1

                Text {
                    text: "Last Toast: " + root.lastToast
                    color: "#f0f6fc"
                    bold: true
                }

                Row {
                    width: implicitWidth
                    height: implicitHeight
                    spacing: 1

                    Button {
                        id: infoBtn

                        focus: true
                        text: "Info"
                        textColor: "#38bdf8"
                        onClicked: root.dispatchToast("Syncing", "Catalog index updated", "info")
                    }

                    Button {
                        id: successBtn

                        text: "Success"
                        textColor: "#2cde85"
                        onClicked: root.dispatchToast("Saved", "Snapshot committed", "success")
                    }
                }

                Row {
                    width: implicitWidth
                    height: implicitHeight
                    spacing: 1

                    Button {
                        id: warningBtn

                        text: "Warning"
                        textColor: "#fbbf24"
                        onClicked: root.dispatchToast("Quota", "Disk usage at 85%", "warning")
                    }

                    Button {
                        id: errorBtn

                        text: "Error"
                        textColor: "#f87171"
                        onClicked: root.dispatchToast("Timeout", "Node rpc unreachable", "error")
                    }
                }
            }

            Toast {
                id: toastHost

                anchors {
                    fill: parent
                    margins: 1
                }
                z: 10
                enabled: false
                toastWidth: 22
                position: root.toastPos
                color: "#161b22"
                textColor: "#c9d1d9"
                infoColor: "#38bdf8"
                successColor: "#2cde85"
                warningColor: "#fbbf24"
                errorColor: "#f87171"
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
                anchors {
                    fill: parent
                    margins: 2
                }
                spacing: 1

                Text {
                    text: "position"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.toastPos
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "Total Sent: " + root.totalSent
                    color: "#58a6ff"
                    bold: true
                }

                Text {
                    text: "toastWidth: " + toastHost.toastWidth
                    color: "#c9d1d9"
                }

                Button {
                    id: posBtn

                    text: "Corner (P)"
                    textColor: "#c9d1d9"
                    onClicked: root.togglePosition()
                }
            }
        }
    }
}
