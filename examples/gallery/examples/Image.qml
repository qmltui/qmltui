pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls
import QmlTui.Image

Item {
    id: root
    property int modeIndex: 0
    property bool smoothScaling: false
    readonly property var renderModes: [Image.HalfBlock, Image.Braille, Image.ASCII, Image.Auto]
    readonly property var modeNames: ["Image.HalfBlock", "Image.Braille", "Image.ASCII", "Image.Auto"]

    function cycleMode() {
        root.modeIndex = (root.modeIndex + 1) % root.renderModes.length;
    }

    function toggleSmooth() {
        root.smoothScaling = !root.smoothScaling;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_M) {
                root.cycleMode();
                event.accepted = true;
            } else if (event.key === Qt.Key_S) {
                root.toggleSmooth();
                event.accepted = true;
            }
        }

        Panel {
            id: stage
            Layout.fillWidth: true
            Layout.fillHeight: true
            focusable: true
            focus: true
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            title: Row {
                spacing: 1
                Text {
                    text: "Image"
                    color: "#f9a8d4"
                }
                Text {
                    text: "[" + root.modeNames[root.modeIndex] + "]"
                    color: "#2cde85"
                }
            }
            titleTrailing: Text {
                text: pandaImage.sourceSize.width + "x" + pandaImage.sourceSize.height + " px"
                color: "#8b949e"
            }
            footer: Text {
                text: "[M] RenderMode  [S] Smooth"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: root.smoothScaling ? "smooth: on" : "smooth: off"
                color: "#58a6ff"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "panda.png · " + root.modeNames[root.modeIndex] + " · PreserveAspectFit"
                    bold: true
                    color: "#c9d1d9"
                }

                Rectangle {
                    width: parent.width
                    height: 14
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    Image {
                        id: pandaImage
                        anchors {
                            fill: parent
                            margins: 1
                        }
                        source: "qrc:/gallery/assets/panda.png"
                        fillMode: Image.PreserveAspectFit
                        renderMode: root.renderModes[root.modeIndex]
                        smooth: root.smoothScaling
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
                    margins: 1
                }
                spacing: 1

                Text {
                    text: "renderMode:"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.modeNames[root.modeIndex]
                    bold: true
                    color: "#f9a8d4"
                }

                Text {
                    text: "sourceSize: " + pandaImage.sourceSize.width + "x" + pandaImage.sourceSize.height
                    bold: true
                    color: "#2cde85"
                }

                Text {
                    text: "renderSize: " + pandaImage.renderSize.width + "x" + pandaImage.renderSize.height
                    color: "#58a6ff"
                }

                Text {
                    text: "smooth: " + pandaImage.smooth
                    color: "#c9d1d9"
                }

                Text {
                    text: "status: " + (pandaImage.status === Image.Ready ? "Image.Ready" : "Loading")
                    color: "#8b949e"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Cycle Mode [M]"
                    onClicked: root.cycleMode()
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Toggle Smooth [S]"
                    onClicked: root.toggleSmooth()
                }
            }
        }
    }
}
