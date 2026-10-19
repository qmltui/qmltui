pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property int borderIndex: 0
    property bool horizontalGradient: true
    readonly property var borderStyles: [Border.Single, Border.Double, Border.Rounded, Border.Ascii]
    readonly property var borderEnums: ["Border.Single", "Border.Double", "Border.Rounded", "Border.Ascii"]
    readonly property var borderColors: ["#58a6ff", "#2cde85", "#d2a8ff", "#f0883e"]

    function cycleBorder() {
        root.borderIndex = (root.borderIndex + 1) % root.borderStyles.length;
    }

    function toggleGradient() {
        root.horizontalGradient = !root.horizontalGradient;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_B) {
                root.cycleBorder();
                event.accepted = true;
            } else if (event.key === Qt.Key_G) {
                root.toggleGradient();
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
                    text: "Rectangle"
                    color: "#60a5fa"
                }
                Text {
                    text: "[Surface Studio]"
                    color: "#2cde85"
                }
            }
            titleTrailing: Text {
                text: root.borderEnums[root.borderIndex]
                color: "#8b949e"
            }
            footer: Text {
                text: "[B] Border  [G] Orientation"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: root.horizontalGradient ? "Grad: Horizontal" : "Grad: Vertical"
                color: "#58a6ff"
            }
            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Row {
                    width: parent.width
                    height: 4
                    spacing: 1

                    Rectangle {
                        width: 8
                        height: 4
                        color: root.borderIndex === 0 ? "#12261e" : "#161b22"
                        border {
                            style: Border.Single
                            color: root.borderIndex === 0 ? "#58a6ff" : "#8b949e"
                        }

                        Text {
                            anchors {
                                centerIn: parent
                            }
                            text: "Single"
                            bold: root.borderIndex === 0
                            color: root.borderIndex === 0 ? "#58a6ff" : "#c9d1d9"
                        }
                    }

                    Rectangle {
                        width: 8
                        height: 4
                        color: root.borderIndex === 1 ? "#12261e" : "#161b22"
                        border {
                            style: Border.Double
                            color: root.borderIndex === 1 ? "#2cde85" : "#8b949e"
                        }

                        Text {
                            anchors {
                                centerIn: parent
                            }
                            text: "Double"
                            bold: root.borderIndex === 1
                            color: root.borderIndex === 1 ? "#2cde85" : "#c9d1d9"
                        }
                    }

                    Rectangle {
                        width: 8
                        height: 4
                        color: root.borderIndex === 2 ? "#12261e" : "#161b22"
                        border {
                            style: Border.Rounded
                            color: root.borderIndex === 2 ? "#d2a8ff" : "#8b949e"
                        }

                        Text {
                            anchors {
                                centerIn: parent
                            }
                            text: "Round"
                            bold: root.borderIndex === 2
                            color: root.borderIndex === 2 ? "#d2a8ff" : "#c9d1d9"
                        }
                    }

                    Rectangle {
                        width: 8
                        height: 4
                        color: root.borderIndex === 3 ? "#12261e" : "#161b22"
                        border {
                            style: Border.Ascii
                            color: root.borderIndex === 3 ? "#f0883e" : "#8b949e"
                        }

                        Text {
                            anchors {
                                centerIn: parent
                            }
                            text: "Ascii"
                            bold: root.borderIndex === 3
                            color: root.borderIndex === 3 ? "#f0883e" : "#c9d1d9"
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 6
                    border {
                        style: root.borderStyles[root.borderIndex]
                        color: root.borderColors[root.borderIndex]
                    }
                    gradient: Gradient {
                        orientation: root.horizontalGradient ? Gradient.Horizontal : Gradient.Vertical

                        GradientStop {
                            position: 0.0
                            color: "#0f2027"
                        }

                        GradientStop {
                            position: 0.5
                            color: "#203a43"
                        }

                        GradientStop {
                            position: 1.0
                            color: "#2c5364"
                        }
                    }

                    Column {
                        anchors {
                            centerIn: parent
                        }
                        spacing: 1

                        Text {
                            text: "Active Surface · " + root.borderEnums[root.borderIndex]
                            bold: true
                            color: "#f0f6fc"
                        }

                        Text {
                            text: "Stops: #0f2027 -> #203a43 -> #2c5364"
                            color: "#7ee0ff"
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 4
                    border {
                        style: Border.Rounded
                        color: "#30363d"
                    }
                    gradient: Gradient {
                        orientation: root.horizontalGradient ? Gradient.Vertical : Gradient.Horizontal

                        GradientStop {
                            position: 0.0
                            color: "#3b1d5e"
                        }

                        GradientStop {
                            position: 1.0
                            color: "#1e3a5f"
                        }
                    }

                    Text {
                        anchors {
                            centerIn: parent
                        }
                        text: "Accent Strip (#3b1d5e -> #1e3a5f)"
                        color: "#e2e8f0"
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
                    text: "border.style:"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.borderEnums[root.borderIndex]
                    bold: true
                    color: root.borderColors[root.borderIndex]
                }

                Text {
                    text: "border.color: " + root.borderColors[root.borderIndex]
                    color: "#c9d1d9"
                }

                Text {
                    text: "gradient.orientation:"
                    color: "#8b949e"
                }

                Text {
                    text: root.horizontalGradient ? "  Gradient.Horizontal" : "  Gradient.Vertical"
                    bold: true
                    color: "#2cde85"
                }

                Text {
                    text: "stops: 0.0 | 0.5 | 1.0"
                    color: "#8b949e"
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Cycle Border [B]"
                    onClicked: root.cycleBorder()
                }

                Button {
                    width: parent.width
                    height: 3
                    text: "Flip Gradient [G]"
                    onClicked: root.toggleGradient()
                }
            }
        }
    }
}
