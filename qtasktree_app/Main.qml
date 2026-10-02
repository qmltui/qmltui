import QtQml
import QTaskTree.App
import Terminal
import Terminal.Controls
import Terminal.Widgets

Terminal {
    id: root

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) {
            Qt.quit()
            event.accepted = true
        } else if (event.key === Qt.Key_R) {
            if (TaskTreeModel.isRunning)
                TaskTreeModel.cancelWorkflow()
            else
                TaskTreeModel.startWorkflow()
            event.accepted = true
        } else if (event.key === Qt.Key_Delete) {
            const nodeId = treeView.currentItemData.nodeId
            if (nodeId)
                TaskTreeModel.removeTask(nodeId)
            event.accepted = true
        }
    }

    Column {
        anchors.fill: parent
        spacing: 0

        Panel {
            width: parent.width
            height: 4
            title: qsTr("Qt TaskTree Monitor")
            titleColor: "cyan"
            titleAlignment: Qt.AlignLeft

            Row {
                x: 2
                y: 1
                spacing: 3

                Text {
                    text: TaskTreeModel.isRunning
                          ? qsTr("STATUS: RUNNING")
                          : qsTr("STATUS: READY")
                    color: TaskTreeModel.isRunning ? "yellow" : "green"
                    bold: true
                }

                Text {
                    text: qsTr("Progress: %1/%2")
                              .arg(TaskTreeModel.progressValue)
                              .arg(TaskTreeModel.progressMaximum)
                    color: "cyan"
                }

                Text {
                    text: qsTr("[R] Run/Stop  [DEL] Remove  [ESC/Q] Quit")
                    color: "gray"
                }
            }
        }

        Row {
            width: parent.width
            height: 3
            spacing: 1

            Button {
                width: 8
                label: TaskTreeModel.isRunning ? qsTr("Stop") : qsTr("Run")
                onClicked: {
                    if (TaskTreeModel.isRunning)
                        TaskTreeModel.cancelWorkflow()
                    else
                        TaskTreeModel.startWorkflow()
                }
            }

            Button {
                width: 12
                label: qsTr("Add Task")
                onClicked: {
                    const parentId = treeView.currentItemData.nodeId || "root"
                    TaskTreeModel.addTask(taskNameInput.text, taskTypeState.type,
                                          parentId, commandInput.text)
                }
            }

            Button {
                width: 10
                label: qsTr("Remove")
                onClicked: {
                    const nodeId = treeView.currentItemData.nodeId
                    if (nodeId)
                        TaskTreeModel.removeTask(nodeId)
                }
            }

            Button {
                width: 16
                label: qsTr("Reset Status")
                onClicked: TaskTreeModel.resetWorkflow()
            }
        }

        Row {
            width: parent.width
            height: Math.max(10, parent.height - 7)
            spacing: 1

            Panel {
                width: Math.floor(parent.width * 0.48)
                height: parent.height
                title: qsTr("Workflow Recipe")
                titleColor: "yellow"

                TreeView {
                    id: treeView

                    x: 1
                    y: 1
                    width: parent.width - 2
                    height: parent.height - 2
                    focus: true
                    borderStyle: "none"
                    model: TaskTreeModel

                    delegate: Item {
                        id: taskDelegate

                        required property int depth
                        required property bool hasChildren
                        required property bool expanded
                        required property bool isCurrent
                        required property string display
                        required property var modelData

                        Row {
                            Text {
                                text: "  ".repeat(taskDelegate.depth)
                            }

                            Text {
                                text: taskDelegate.hasChildren
                                      ? (taskDelegate.expanded ? "[-] " : "[+] ")
                                      : "    "
                                color: "cyan"
                            }

                            Text {
                                text: taskDelegate.modelData.statusIcon + " "
                                color: taskDelegate.modelData.statusColor
                            }

                            Text {
                                text: taskDelegate.display
                                color: taskDelegate.isCurrent ? "white" : "gray"
                                bold: taskDelegate.isCurrent
                            }

                            Text {
                                text: taskDelegate.modelData.duration
                                      ? " (" + taskDelegate.modelData.duration + ")"
                                      : ""
                                color: "yellow"
                            }
                        }
                    }

                    onItemActivated: {
                        const nodeId = treeView.currentItemData.nodeId
                        if (nodeId)
                            TaskTreeModel.selectTask(nodeId)
                    }
                    Component.onCompleted: treeView.expand(0)
                }
            }

            Panel {
                width: parent.width - Math.floor(parent.width * 0.48) - 1
                height: parent.height
                title: qsTr("Task Inspector")
                titleColor: "green"

                Column {
                    x: 1
                    y: 1
                    width: parent.width - 2
                    spacing: 1

                    Text {
                        text: TaskTreeModel.activeTaskLog || qsTr("No task selected.")
                        color: "cyan"
                    }

                    Text {
                        visible: TaskTreeModel.lastError.length > 0
                        text: TaskTreeModel.lastError
                        color: "red"
                    }

                    Text {
                        text: "----------------------------------------------"
                        color: "gray"
                    }

                    Text {
                        text: qsTr("CREATE NEW TASK")
                        color: "yellow"
                        bold: true
                    }

                    Row {
                        spacing: 1

                        Text {
                            text: qsTr("Name: ")
                            color: "gray"
                        }

                        TextInput {
                            id: taskNameInput

                            text: qsTr("New build step")
                            color: "white"
                        }
                    }

                    Row {
                        spacing: 1

                        Text {
                            text: qsTr("Type: ")
                            color: "gray"
                        }

                        Text {
                            id: taskTypeState

                            property string type: "QProcessTask"

                            text: "[" + type + "]"
                            color: "yellow"

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (taskTypeState.type === "QProcessTask")
                                        taskTypeState.type = "GroupSequential"
                                    else if (taskTypeState.type === "GroupSequential")
                                        taskTypeState.type = "GroupParallel"
                                    else
                                        taskTypeState.type = "QProcessTask"
                                }
                            }
                        }
                    }

                    Row {
                        spacing: 1

                        Text {
                            text: qsTr("Command: ")
                            color: "gray"
                        }

                        TextInput {
                            id: commandInput

                            text: "cmake --build build"
                            color: "white"
                        }
                    }

                    Text {
                        text: qsTr("Select a group, then choose Add Task.")
                        color: "gray"
                    }
                }
            }
        }
    }
}
