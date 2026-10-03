import QtQuick
import Qt.labs.qmlmodels
import Qtui

Terminal {
    id: root

    enum MessageType {
        SystemMessage,
        UserMessage,
        AgentMessage,
        ThoughtMessage,
        ToolCallMessage,
        ToolResultMessage,
        PermissionMessage,
        ErrorMessage
    }


    enum AgentState {
        ReadyState,
        WorkingState,
        PermissionState
    }

    rendererMode: Terminal.Stream
    fillTerminal: false
    useAlternateScreen: false
    mouseReporting: false
    implicitHeight: awaitingPermission ? 6 : 2

    property int agentState: Main.ReadyState
    property int agentStep: 0
    property int permissionIndex: 0
    property int spinnerIndex: 0
    property string requestText: ""
    readonly property bool ready: agentState === Main.ReadyState
    readonly property bool working: agentState === Main.WorkingState
    readonly property bool awaitingPermission: agentState === Main.PermissionState
    readonly property var spinnerFrames: ["|", "/", "-", "\\"]
    readonly property string workingLabel: {
        if (agentStep <= 1)
            return qsTr("Analyzing the request")
        if (agentStep <= 3)
            return qsTr("Reading project files")
        if (agentStep <= 6)
            return qsTr("Applying changes")
        return qsTr("Running verification")
    }

    function addMessage(messageType, content) {
        chatHistory.append({
            "type": messageType,
            "content": content
        })
    }

    function submitRequest() {
        const request = input.text.trim()
        if (request.length === 0 || !ready)
            return

        requestText = request
        input.text = ""
        agentStep = 0
        agentState = Main.WorkingState
        addMessage(Main.UserMessage, request)
        agentTimer.start()
    }

    function choosePermission(index) {
        permissionIndex = index
        if (index === 2) {
            addMessage(Main.PermissionMessage, qsTr("Denied file modification"))
            addMessage(Main.ErrorMessage, qsTr("Stopped before changing any files."))
            agentState = Main.ReadyState
            return
        }

        const decision = index === 0
                       ? qsTr("Allowed file modification once")
                       : qsTr("Allowed file modification for this session")
        addMessage(Main.PermissionMessage, decision)
        agentStep = 5
        agentState = Main.WorkingState
        agentTimer.start()
    }

    function interruptAgent() {
        agentTimer.stop()
        addMessage(Main.ErrorMessage, qsTr("Interrupted by user."))
        agentState = Main.ReadyState
    }

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
            return
        }

        if ((event.modifiers & Qt.ControlModifier) && event.key === Qt.Key_C) {
            if (root.ready)
                Qt.quit()
            else
                root.interruptAgent()
            event.accepted = true
            return
        }

        if (!root.awaitingPermission)
            return

        if (event.key === Qt.Key_Up) {
            root.permissionIndex = Math.max(0, root.permissionIndex - 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            root.permissionIndex = Math.min(2, root.permissionIndex + 1)
            event.accepted = true
        } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            root.choosePermission(root.permissionIndex)
            event.accepted = true
        }
    }

    ListModel {
        id: chatHistory
    }

    StreamAppender {
        model: chatHistory

        delegate: DelegateChooser {
            role: "type"

            DelegateChoice {
                roleValue: Main.SystemMessage

                Text {
                    required property int index
                    required property int type
                    required property string content

                    text: content
                    color: "gray"
                }
            }

            DelegateChoice {
                roleValue: Main.UserMessage

                Text {
                    required property int index
                    required property int type
                    required property string content

                    text: qsTr("> %1").arg(content)
                    color: "cyan"
                    bold: true
                }
            }

            DelegateChoice {
                roleValue: Main.AgentMessage

                Column {
                    id: agentDelegate
                    required property int index
                    required property int type
                    required property string content

                    Text {
                        text: qsTr("Agent")
                        color: "magenta"
                        bold: true
                    }

                    Text {
                        text: agentDelegate.content
                        color: "white"
                    }
                }
            }

            DelegateChoice {
                roleValue: Main.ThoughtMessage

                Text {
                    required property int index
                    required property int type
                    required property string content

                    text: qsTr("* %1").arg(content)
                    color: "gray"
                    italic: true
                }
            }

            DelegateChoice {
                roleValue: Main.ToolCallMessage

                Column {
                    id: toolCallDelegate
                    required property int index
                    required property int type
                    required property string content

                    Text {
                        text: qsTr("Tool call")
                        color: "cyan"
                        bold: true
                    }

                    Text {
                        text: qsTr("  %1").arg(toolCallDelegate.content)
                        color: "white"
                    }
                }
            }

            DelegateChoice {
                roleValue: Main.ToolResultMessage

                Text {
                    required property int index
                    required property int type
                    required property string content

                    text: qsTr("+ %1").arg(content)
                    color: "green"
                }
            }

            DelegateChoice {
                roleValue: Main.PermissionMessage

                Text {
                    required property int index
                    required property int type
                    required property string content

                    text: qsTr("! %1").arg(content)
                    color: "yellow"
                    bold: true
                }
            }

            DelegateChoice {
                roleValue: Main.ErrorMessage

                Text {
                    required property int index
                    required property int type
                    required property string content

                    text: qsTr("x %1").arg(content)
                    color: "red"
                    bold: true
                }
            }
        }
    }

    Column {
        id: promptLayout
        width: root.width

        Text {
            text: root.ready
                  ? qsTr(" qtui code  |  ready")
                  : root.awaitingPermission
                    ? qsTr(" qtui code  |  permission required")
                    : qsTr(" qtui code  |  working")
            color: root.ready ? "cyan" : root.awaitingPermission ? "yellow" : "magenta"
            bold: true
        }

        Row {
            visible: root.working
            width: root.width

            Text {
                text: root.spinnerFrames[root.spinnerIndex]
                color: "magenta"
                bold: true
            }

            Text {
                text: qsTr(" %1 · Ctrl+C to interrupt").arg(root.workingLabel)
                color: "gray"
            }
        }

        Column {
            visible: root.awaitingPermission
            width: root.width

            Text {
                text: qsTr("The agent wants to edit Tui/src/engine/StreamRenderer.cpp")
                color: "white"
                bold: true
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.permissionIndex === 0 ? "yellow" : "transparent"
                Text {
                    text: (root.permissionIndex === 0 ? "> " : "  ") + qsTr("Allow once")
                    color: root.permissionIndex === 0 ? "black" : "white"
                    bold: root.permissionIndex === 0
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.permissionIndex === 1 ? "yellow" : "transparent"
                Text {
                    text: (root.permissionIndex === 1 ? "> " : "  ")
                          + qsTr("Always allow for this session")
                    color: root.permissionIndex === 1 ? "black" : "white"
                    bold: root.permissionIndex === 1
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: root.permissionIndex === 2 ? "yellow" : "transparent"
                Text {
                    text: (root.permissionIndex === 2 ? "> " : "  ") + qsTr("Deny")
                    color: root.permissionIndex === 2 ? "black" : "white"
                    bold: root.permissionIndex === 2
                }
            }

            Text {
                text: qsTr("Use Up/Down and Enter · Ctrl+C to cancel")
                color: "gray"
            }
        }

        Row {
            visible: root.ready
            width: root.width

            Text {
                text: qsTr("> ")
                color: "green"
                bold: true
            }

            TextInput {
                id: input
                width: parent.width - 2
                color: "white"
                focus: root.ready
                onAccepted: root.submitRequest()
            }
        }
    }

    Timer {
        id: spinnerTimer
        interval: 90
        running: root.working
        repeat: true
        onTriggered: root.spinnerIndex = (root.spinnerIndex + 1) % root.spinnerFrames.length
    }

    Timer {
        id: agentTimer
        interval: 650
        repeat: true

        onTriggered: {
            if (root.agentStep === 0) {
                root.addMessage(Main.ThoughtMessage, qsTr("Inspecting the repository structure"))
            } else if (root.agentStep === 1) {
                root.addMessage(Main.ToolCallMessage,
                                qsTr("Read Tui/src/engine/StreamRenderer.cpp"))
            } else if (root.agentStep === 2) {
                root.addMessage(Main.ToolResultMessage, qsTr("Read 152 lines"))
            } else if (root.agentStep === 3) {
                root.addMessage(Main.AgentMessage,
                                qsTr("I found the renderer. I need permission to update it."))
            } else if (root.agentStep === 4) {
                root.addMessage(Main.PermissionMessage,
                                qsTr("Requested permission to modify one source file"))
                root.permissionIndex = 0
                root.agentState = Main.PermissionState
                agentTimer.stop()
            } else if (root.agentStep === 5) {
                root.addMessage(Main.ToolCallMessage,
                                qsTr("Edit Tui/src/engine/StreamRenderer.cpp"))
            } else if (root.agentStep === 6) {
                root.addMessage(Main.ToolResultMessage, qsTr("Updated 14 lines"))
            } else if (root.agentStep === 7) {
                root.addMessage(Main.ToolCallMessage, qsTr("Run cmake --build build"))
            } else if (root.agentStep === 8) {
                root.addMessage(Main.ToolResultMessage, qsTr("Build succeeded · 6/6 tests passed"))
            } else {
                root.addMessage(Main.AgentMessage,
                                qsTr("Implemented the change and verified the test suite."))
                root.agentState = Main.ReadyState
                agentTimer.stop()
            }
            root.agentStep += 1
        }
    }

    Component.onCompleted: {
        root.addMessage(Main.SystemMessage, qsTr("Qtui Code"))
        root.addMessage(Main.SystemMessage,
                        qsTr("Model-backed stream · typed delegates · interactive permissions"))
        root.addMessage(Main.SystemMessage,
                        qsTr("Enter sends · Ctrl+C interrupts work or quits when idle"))
    }
}
