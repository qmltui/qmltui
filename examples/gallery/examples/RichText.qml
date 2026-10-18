pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root
    property int themeIndex: 0
    property bool boldBase: false
    property bool wrapRich: true
    property string lastLink: "none"
    readonly property var themes: [
        {
            name: "Emerald",
            hex: "#2cde85"
        },
        {
            name: "Cyan",
            hex: "#38bdf8"
        },
        {
            name: "Amber",
            hex: "#fbbf24"
        }
    ]
    readonly property string accentName: themes[themeIndex].name
    readonly property string accentHex: themes[themeIndex].hex

    function cycleTheme() {
        root.themeIndex = (root.themeIndex + 1) % root.themes.length;
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_T) {
                root.cycleTheme();
                event.accepted = true;
            } else if (event.key === Qt.Key_B) {
                root.boldBase = !root.boldBase;
                event.accepted = true;
            } else if (event.key === Qt.Key_W) {
                root.wrapRich = !root.wrapRich;
                event.accepted = true;
            }
        }

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "RichText · Spans, HTML & ANSI"
            titleColor: "#f8fafc"
            titleBadge: "[CORE]"
            titleBadgeColor: "#fde68a"
            titleTrailing: root.accentName
            titleTrailingColor: root.accentHex
            footer: "T theme · B bold · W wrap · click link"
            footerColor: "#8b949e"
            footerTrailing: "3 formats"
            footerTrailingColor: "#58a6ff"
            color: "#0d1117"
            border {
                style: Border.Rounded
                color: "#30363d"
            }

            Column {
                anchors {
                    fill: parent
                    margins: 1
                }
                spacing: 1

                Rectangle {
                    width: parent.width
                    height: 4
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    Text {
                        x: 1
                        y: 1
                        text: "1. Text.spans + QmlTui.span()"
                        bold: true
                        color: "#fde68a"
                    }

                    Text {
                        x: 1
                        y: 2
                        bold: root.boldBase
                        spans: [QmlTui.span(" SPAN ", {
                                color: "#0d1117",
                                backgroundColor: root.accentHex,
                                bold: true
                            }),
                            {
                                text: " literal ",
                                color: "#f8fafc"
                            },
                            QmlTui.span("underlined", {
                                color: "#fbbf24",
                                underline: true,
                                bold: false
                            }), QmlTui.span(" · dim", {
                                dim: true
                            })]
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 5
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    Text {
                        x: 1
                        y: 1
                        text: "2. Text.RichText (HTML + Hyperlink)"
                        bold: true
                        color: "#38bdf8"
                    }

                    Text {
                        x: 1
                        y: 2
                        width: Math.max(10, parent.width - 2)
                        textFormat: Text.RichText
                        wrapMode: root.wrapRich ? Text.WordWrap : Text.NoWrap
                        text: "Tags: <b>bold</b> <i>italic</i> <u>underline</u> <font color=\"" + root.accentHex + "\">accent</font> <a href=\"https://qt.io\">[qt.io]</a> [docs](https://qmltui.org)"
                        color: "#c9d1d9"
                        onLinkActivated: link => {
                            root.lastLink = link;
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 4
                    color: "#161b22"
                    border {
                        style: Border.Single
                        color: "#30363d"
                    }

                    Text {
                        x: 1
                        y: 1
                        text: "3. Text.AnsiText (VT SGR Escape Codes)"
                        bold: true
                        color: "#c084fc"
                    }

                    Text {
                        x: 1
                        y: 2
                        textFormat: Text.AnsiText
                        text: "\u001b[1;32m[OK]\u001b[0m \u001b[1;36mSGR-CYAN\u001b[0m \u001b[4;33mUNDERLINE\u001b[0m \u001b[7mREVERSED\u001b[0m"
                    }
                }
            }
        }

        Panel {
            Layout.preferredWidth: 27
            Layout.fillHeight: true
            title: "Inspector"
            titleColor: "#58a6ff"
            footer: "RichText API"
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
                    text: "FORMAT PIPELINE"
                    bold: true
                    color: "#fde68a"
                }

                Text {
                    text: "1: QmlTui.span[]"
                    color: "#c9d1d9"
                }

                Text {
                    text: "2: Text.RichText"
                    color: "#c9d1d9"
                }

                Text {
                    text: "3: Text.AnsiText"
                    color: "#c9d1d9"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "STATE & TELEMETRY"
                    bold: true
                    color: "#58a6ff"
                }

                Text {
                    text: "Theme:    " + root.accentName
                    color: root.accentHex
                }

                Text {
                    text: "BoldBase: " + (root.boldBase ? "true" : "false")
                    color: root.boldBase ? "#2cde85" : "#8b949e"
                }

                Text {
                    text: "RichWrap: " + (root.wrapRich ? "WordWrap" : "NoWrap")
                    color: "#38bdf8"
                }

                Text {
                    width: parent.width
                    text: "Link:     " + root.lastLink
                    color: "#fbbf24"
                }

                Rectangle {
                    width: parent.width
                    height: 1
                    color: "transparent"
                }

                Text {
                    text: "CONTROLS"
                    bold: true
                    color: "#58a6ff"
                }

                Button {
                    width: parent.width
                    height: 1
                    focus: true
                    text: "[ Theme: " + root.accentName + " (T) ]"
                    textColor: root.accentHex
                    onClicked: root.cycleTheme()
                }

                CheckBox {
                    text: "Base bold (B)"
                    checked: root.boldBase
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.boldBase = checked;
                    }
                }

                CheckBox {
                    text: "WordWrap (W)"
                    checked: root.wrapRich
                    textColor: "#c9d1d9"
                    onToggled: checked => {
                        root.wrapRich = checked;
                    }
                }
            }
        }
    }
}
