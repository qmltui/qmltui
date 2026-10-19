pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property real stepSize: 1
    property bool showValue: true
    property bool slidersEnabled: true
    readonly property int masterLevel: Math.round((volSlider.value + bassSlider.value + trebleSlider.value) / 3)

    function resetMix() {
        volSlider.value = 42;
        bassSlider.value = 60;
        trebleSlider.value = 50;
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
                    text: "Slider"
                    color: "#c084fc"
                }
                Text {
                    text: "Controls"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: "MIX " + root.masterLevel + "%"
                color: "#2cde85"
            }
            footer: Text {
                text: "Left/Right/Home/End adjust"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "Tab channel"
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
                    text: "Audio & Channel Mixer"
                    color: "#c9d1d9"
                    bold: true
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        text: "Volume  " + Math.round(volSlider.value) + "%"
                        color: "#c084fc"
                        bold: volSlider.activeFocus
                    }

                    Slider {
                        id: volSlider

                        width: parent.width
                        focus: true
                        enabled: root.slidersEnabled
                        from: 0
                        to: 100
                        value: 42
                        stepSize: root.stepSize
                        showValue: root.showValue
                        textColor: "#c084fc"
                    }
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        text: "Bass    " + Math.round(bassSlider.value) + "%"
                        color: "#38bdf8"
                        bold: bassSlider.activeFocus
                    }

                    Slider {
                        id: bassSlider

                        width: parent.width
                        enabled: root.slidersEnabled
                        from: 0
                        to: 100
                        value: 60
                        stepSize: root.stepSize
                        showValue: root.showValue
                        textColor: "#38bdf8"
                    }
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        text: "Treble  " + Math.round(trebleSlider.value) + "%"
                        color: "#2cde85"
                        bold: trebleSlider.activeFocus
                    }

                    Slider {
                        id: trebleSlider

                        width: parent.width
                        enabled: root.slidersEnabled
                        from: 0
                        to: 100
                        value: 50
                        stepSize: root.stepSize
                        showValue: root.showValue
                        textColor: "#2cde85"
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
                        text: "Master Bus: " + root.masterLevel + "%  |  Step: " + root.stepSize
                        color: "#f9c74f"
                        bold: true
                    }
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
                        checked: root.showValue
                        text: "showValue"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.showValue = checked;
                        }
                    }

                    CheckBox {
                        checked: root.stepSize === 5
                        text: "stepSize: 5"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.stepSize = checked ? 5 : 1;
                        }
                    }

                    CheckBox {
                        checked: root.slidersEnabled
                        text: "enabled"
                        textColor: "#c9d1d9"

                        onToggled: checked => {
                            root.slidersEnabled = checked;
                        }
                    }
                }

                Button {
                    text: "Reset EQ"
                    textColor: "#f9c74f"

                    onClicked: {
                        root.resetMix();
                    }
                }

                Column {
                    width: parent.width
                    spacing: 0

                    Text {
                        text: "LEVEL METER"
                        color: "#58a6ff"
                        bold: true
                    }

                    ProgressBar {
                        width: parent.width
                        from: 0
                        to: 100
                        value: root.masterLevel
                        style: "segmented"
                        showPercentage: true
                        color: "#2cde85"
                    }

                    Text {
                        text: "volume: " + Math.round(volSlider.value) + "%"
                        color: "#c084fc"
                    }

                    Text {
                        text: "bass: " + Math.round(bassSlider.value) + "%"
                        color: "#38bdf8"
                    }

                    Text {
                        text: "treble: " + Math.round(trebleSlider.value) + "%"
                        color: "#2cde85"
                    }

                    Text {
                        text: "stepSize: " + root.stepSize
                        color: "#8b949e"
                    }
                }
            }
        }
    }
}
