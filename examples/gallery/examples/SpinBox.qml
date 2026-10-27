pragma ComponentBehavior: Bound
import QmlTui
import QmlTui.Controls

// SpinBox: Default inline stepper, stepSize range stepper, and custom-styled
// SpinBox using border, downIndicator, contentItem, and upIndicator slots.
Column {
    id: spinDemo

    anchors.centerIn: parent
    spacing: 1

    Text {
        text: "Up/Down or [-] / [+] steps the value:"
        color: "#8b949e"
    }

    // Default inline SpinBox
    Row {
        spacing: 2

        Text {
            width: 14
            text: "Workers"
            color: "#c9d1d9"
            bold: true
        }

        SpinBox {
            id: workersSpin
            width: 18
            from: 1
            to: 32
            value: 4
            focus: true
            textColor: "#2cde85"
        }
    }

    // Step and range SpinBox
    Row {
        spacing: 2

        Text {
            width: 14
            text: "Port"
            color: "#c9d1d9"
            bold: true
        }

        SpinBox {
            id: portSpin
            width: 18
            from: 1000
            to: 9000
            stepSize: 100
            value: 8080
            textColor: "#58a6ff"
        }
    }

    // Custom-styled SpinBox with Rounded/Double border and custom indicators
    Row {
        spacing: 2

        Text {
            y: 1
            width: 14
            text: "Custom Style"
            color: "#c9d1d9"
            bold: true
        }

        SpinBox {
            id: timeoutSpin
            width: 18
            from: 5
            to: 120
            stepSize: 5
            value: 30
            suffix: " s"
            wrap: true
            textColor: "#f59e0b"
            border {
                style: timeoutSpin.activeFocus ? Border.Double : Border.Rounded
                color: timeoutSpin.activeFocus ? "#fbbf24" : "#f59e0b"
            }
            downIndicator: Text {
                text: "◄"
                color: "#f59e0b"
                bold: timeoutSpin.activeFocus
            }
            contentItem: Text {
                text: timeoutSpin.displayText
                horizontalAlignment: Qt.AlignHCenter
                color: "#f59e0b"
                bold: timeoutSpin.activeFocus
                reversed: timeoutSpin.activeFocus
            }
            upIndicator: Text {
                text: "►"
                color: "#f59e0b"
                bold: timeoutSpin.activeFocus
            }
        }
    }

    Text {
        text: "workers=" + workersSpin.value + "  port=" + portSpin.value + "  timeout=" + timeoutSpin.displayText
        color: "#2cde85"
    }
}
