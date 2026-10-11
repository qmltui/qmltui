import QtQml
import Qamui
import Qamui.Controls
import Qamui.Examples.DeviceCockpit

// Static IPv4 editor. Tab cycles the fields, Enter applies, Esc cancels.
Popup {
    id: control

    property string interfaceName: ""

    signal applyRequested(string address, int prefix, string gateway)

    function openFor(name, address, prefix, gateway) {
        interfaceName = name
        addressInput.text = address
        prefixInput.text = prefix > 0 ? String(prefix) : "24"
        gatewayInput.text = gateway
        open()
    }

    function applyNow() {
        const prefix = parseInt(prefixInput.text)
        control.applyRequested(addressInput.text,
                               isNaN(prefix) ? 24 : prefix,
                               gatewayInput.text)
        control.close()
    }

    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape
    implicitWidth: 46
    implicitHeight: contentColumn.implicitHeight + 2
    border.style: "double"
    border.color: Theme.borderFocus
    color: Theme.bgHeader
    anchors.centerIn: parent
    initialFocusItem: addressInput

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
            const fields = [addressInput, prefixInput, gatewayInput]
            let current = 0
            for (let i = 0; i < fields.length; ++i) {
                if (fields[i].activeFocus)
                    current = i
            }
            const step = event.key === Qt.Key_Backtab ? fields.length - 1 : 1
            fields[(current + step) % fields.length].focus = true
            event.accepted = true
        }
    }

    Column {
        id: contentColumn
        x: 2
        y: 1
        spacing: 0

        Text {
            text: "STATIC IPV4 — " + control.interfaceName
            bold: true
            color: Theme.textWarning
        }

        Row {
            spacing: 1
            Text { width: 10; text: "Address"; color: Theme.textSecondary }
            Rectangle {
                width: 28
                height: 3
                color: Theme.bgPanel
                border.style: addressInput.activeFocus ? "double" : "single"
                border.color: addressInput.activeFocus ? Theme.borderFocus : Theme.borderNormal
                TextInput {
                    id: addressInput
                    anchors.fill: parent
                    anchors.margins: 1
                    color: Theme.textPrimary
                    onAccepted: control.applyNow()
                }
            }
        }
        Row {
            spacing: 1
            Text { width: 10; text: "Prefix"; color: Theme.textSecondary }
            Rectangle {
                width: 28
                height: 3
                color: Theme.bgPanel
                border.style: prefixInput.activeFocus ? "double" : "single"
                border.color: prefixInput.activeFocus ? Theme.borderFocus : Theme.borderNormal
                TextInput {
                    id: prefixInput
                    anchors.fill: parent
                    anchors.margins: 1
                    color: Theme.textPrimary
                    onAccepted: control.applyNow()
                }
            }
        }
        Row {
            spacing: 1
            Text { width: 10; text: "Gateway"; color: Theme.textSecondary }
            Rectangle {
                width: 28
                height: 3
                color: Theme.bgPanel
                border.style: gatewayInput.activeFocus ? "double" : "single"
                border.color: gatewayInput.activeFocus ? Theme.borderFocus : Theme.borderNormal
                TextInput {
                    id: gatewayInput
                    anchors.fill: parent
                    anchors.margins: 1
                    color: Theme.textPrimary
                    onAccepted: control.applyNow()
                }
            }
        }

        Row {
            spacing: 2

            Button {
                text: "[Enter] Apply"
                onClicked: control.applyNow()
            }
            Button {
                text: "[Esc] Cancel"
                onClicked: control.close()
            }
        }
    }
}
