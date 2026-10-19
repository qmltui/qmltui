pragma ComponentBehavior: Bound
import QtQml
import QmlTui
import QmlTui.Controls

Item {
    id: root

    property string deployStatus: "Configuring"

    readonly property string activeStepTitle: {
        if (stack.depth === 1) {
            return "Environment";
        }
        if (stack.depth === 2) {
            return "Replicas";
        }
        return "Confirm";
    }

    function pushStep() {
        if (stack.depth === 1) {
            stack.push(step2Comp);
            deployStatus = "Sizing replicas";
        } else if (stack.depth === 2) {
            stack.push(step3Comp);
            deployStatus = "Ready to commit";
        } else {
            deployStatus = "Deployed!";
        }
    }

    function popStep() {
        if (stack.depth > 1) {
            stack.pop();
            deployStatus = (stack.depth === 1) ? "Configuring" : "Sizing replicas";
        }
    }

    function resetStack() {
        while (stack.depth > 1) {
            stack.pop();
        }
        deployStatus = "Configuring";
    }

    Component.onCompleted: {
        stack.push(step1Comp);
    }

    Component {
        id: step1Comp

        Panel {
            color: "#161b22"
            border {
                style: Border.Single
                color: "#2cde85"
            }

            title: Text {
                text: "Step 1: Environment"
                color: "#2cde85"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Target Cluster: prod-eu-west-1"
                    color: "#f0f6fc"
                    bold: true
                }

                Text {
                    text: "Namespace:      payments-core"
                    color: "#58a6ff"
                }

                Text {
                    text: "Press Push / Next to configure replicas."
                    color: "#8b949e"
                }
            }
        }
    }

    Component {
        id: step2Comp

        Panel {
            color: "#161b22"
            border {
                style: Border.Single
                color: "#fbbf24"
            }

            title: Text {
                text: "Step 2: Replicas"
                color: "#fbbf24"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Desired Pods:   6 replicas (HPA)"
                    color: "#f0f6fc"
                    bold: true
                }

                Text {
                    text: "Strategy:       RollingUpdate (25%)"
                    color: "#fbbf24"
                }

                Text {
                    text: "Press Push for final confirmation."
                    color: "#8b949e"
                }
            }
        }
    }

    Component {
        id: step3Comp

        Panel {
            color: "#161b22"
            border {
                style: Border.Double
                color: "#e879f9"
            }

            title: Text {
                text: "Step 3: Confirm"
                color: "#e879f9"
            }
            Column {
                x: 2
                y: 2
                spacing: 1

                Text {
                    text: "Ready to deploy 6 pods to prod"
                    color: "#f0f6fc"
                    bold: true
                }

                Text {
                    text: "Image:          registry.io/pay:v2.4.0"
                    color: "#2cde85"
                }

                Text {
                    text: "Pop to revise or Reset to restart."
                    color: "#8b949e"
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 1

        Keys.onPressed: event => {
            if (event.key === Qt.Key_N) {
                root.pushStep();
                event.accepted = true;
            } else if (event.key === Qt.Key_P) {
                root.popStep();
                event.accepted = true;
            } else if (event.key === Qt.Key_R) {
                root.resetStack();
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
                    text: "StackView Wizard"
                    color: "#e879f9"
                }
                Text {
                    text: "[Controls]"
                    color: "#58a6ff"
                }
            }
            titleTrailing: Text {
                text: "Depth " + stack.depth + "/3"
                color: "#2cde85"
            }
            footer: Text {
                text: "N push · P pop · R reset"
                color: "#8b949e"
            }
            footerTrailing: Text {
                text: "StackView"
                color: "#8b949e"
            }
            StackView {
                id: stack

                x: 2
                y: 2
                width: Math.max(24, parent.width - 4)
                height: 9
            }

            Row {
                x: 2
                y: 12
                spacing: 1

                Button {
                    id: pushBtn

                    focus: true
                    text: "Push"
                    textColor: "#2cde85"
                    onClicked: root.pushStep()
                }

                Button {
                    id: popBtn

                    text: "Pop"
                    textColor: "#fbbf24"
                    onClicked: root.popStep()
                }

                Button {
                    id: resetBtn

                    text: "Reset"
                    textColor: "#f87171"
                    onClicked: root.resetStack()
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
                    text: "stack.depth: " + stack.depth
                    color: "#2cde85"
                    bold: true
                }

                Text {
                    text: "Active Step:"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.activeStepTitle
                    color: "#58a6ff"
                    bold: true
                }

                Text {
                    text: "Status:"
                    color: "#8b949e"
                }

                Text {
                    text: "  " + root.deployStatus
                    color: "#f0f6fc"
                }
            }
        }
    }
}
