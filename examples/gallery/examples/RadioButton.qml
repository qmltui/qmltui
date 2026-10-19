pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property bool radiosEnabled: true
    property bool stripSymbols: false
    readonly property string selectedProfile: {
        if (ultraRadio.checked)
            return "Ultra";
        if (sizeRadio.checked)
            return "Size";
        if (releaseRadio.checked)
            return "Release";
        return "Debug";
    }
    readonly property string compilerFlags: {
        let base = "-O0 -g";
        if (ultraRadio.checked)
            base = "-O3 -flto";
        else if (sizeRadio.checked)
            base = "-Os";
        else if (releaseRadio.checked)
            base = "-O2";
        return stripSymbols ? base + " -s" : base;
    }
    readonly property string binarySize: {
        if (ultraRadio.checked)
            return stripSymbols ? "4.1 MB" : "5.1 MB";
        if (sizeRadio.checked)
            return stripSymbols ? "2.1 MB" : "2.6 MB";
        if (releaseRadio.checked)
            return stripSymbols ? "3.4 MB" : "4.2 MB";
        return stripSymbols ? "8.2 MB" : "14.8 MB";
    }
    readonly property string buildTime: {
        if (ultraRadio.checked)
            return "9.4s";
        if (sizeRadio.checked)
            return "4.1s";
        if (releaseRadio.checked)
            return "3.8s";
        return "1.2s";
    }

    ButtonGroup {
        id: profileGroup

        buttons: [debugRadio, releaseRadio, sizeRadio, ultraRadio]
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_1) {
                debugRadio.checked = true;
                event.accepted = true;
            } else if (event.key === Qt.Key_2) {
                releaseRadio.checked = true;
                event.accepted = true;
            } else if (event.key === Qt.Key_3) {
                sizeRadio.checked = true;
                event.accepted = true;
            } else if (event.key === Qt.Key_4) {
                ultraRadio.checked = true;
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
                    text: "RadioButton"
                    color: "#f9c74f"
                }
                Text {
                    text: "Controls"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: root.selectedProfile
                color: "#2cde85"
            }
            footer: Text {
                text: "Space select  1-4 jump"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "Tab next"
                color: "#8b949e"
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
                    text: "Build Profile Selector (ButtonGroup)"
                    color: "#c9d1d9"
                    bold: true
                }

                Column {
                    spacing: 1

                    RadioButton {
                        id: debugRadio

                        focus: true
                        checked: true
                        enabled: root.radiosEnabled
                        hoverEnabled: true
                        buttonGroup: profileGroup
                        text: "Debug (-O0 -g)"
                        textColor: checked ? "#f9c74f" : "#c9d1d9"
                    }

                    RadioButton {
                        id: releaseRadio

                        enabled: root.radiosEnabled
                        hoverEnabled: true
                        buttonGroup: profileGroup
                        text: "Release (-O2)"
                        textColor: checked ? "#f9c74f" : "#c9d1d9"
                    }

                    RadioButton {
                        id: sizeRadio

                        enabled: root.radiosEnabled
                        hoverEnabled: true
                        buttonGroup: profileGroup
                        text: "Size (-Os)"
                        textColor: checked ? "#f9c74f" : "#c9d1d9"
                    }

                    RadioButton {
                        id: ultraRadio

                        enabled: root.radiosEnabled
                        hoverEnabled: true
                        buttonGroup: profileGroup
                        text: "Ultra (-O3 -flto)"
                        textColor: checked ? "#f9c74f" : "#c9d1d9"
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 3
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    Text {
                        anchors {
                            verticalCenter: parent.verticalCenter
                            left: parent.left
                            leftMargin: 2
                        }
                        text: "c++ " + root.compilerFlags + " main.cpp"
                        color: "#2cde85"
                        bold: true
                    }
                }

                Text {
                    text: "Selected: " + root.selectedProfile + "  |  Est. binary: " + root.binarySize
                    color: "#7ee0ff"
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
                anchors {
                    fill: parent
                    leftMargin: 2
                    rightMargin: 2
                    topMargin: 1
                    bottomMargin: 1
                }
                spacing: 1

                Column {
                    spacing: 0

                    Text {
                        text: "PROPERTIES"
                        color: "#58a6ff"
                        bold: true
                    }

                    CheckBox {
                        checked: root.radiosEnabled
                        text: "enabled"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.radiosEnabled = checked;
                        }
                    }

                    CheckBox {
                        checked: root.stripSymbols
                        text: "strip (-s)"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.stripSymbols = checked;
                        }
                    }
                }

                Button {
                    text: "Cycle Profile"
                    textColor: "#f9c74f"

                    onClicked: {
                        if (debugRadio.checked)
                            releaseRadio.checked = true;
                        else if (releaseRadio.checked)
                            sizeRadio.checked = true;
                        else if (sizeRadio.checked)
                            ultraRadio.checked = true;
                        else
                            debugRadio.checked = true;
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
                        text: "profile: " + root.selectedProfile
                        color: "#f9c74f"
                        bold: true
                    }

                    Text {
                        text: "flags: " + root.compilerFlags
                        color: "#2cde85"
                    }

                    Text {
                        text: "size: " + root.binarySize
                        color: "#7ee0ff"
                    }

                    Text {
                        text: "time: " + root.buildTime
                        color: "#c9d1d9"
                    }

                    Text {
                        text: "exclusive: true"
                        color: "#8b949e"
                    }
                }
            }
        }
    }
}
