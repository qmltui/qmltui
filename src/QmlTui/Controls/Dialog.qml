import QtQml
import QmlTui

// Centered modal confirmation Popup with a double border, title, body message,
// and OK / Cancel buttons. Enter triggers accept unless the reject button has
// active focus; Escape rejects via Popup.CloseOnEscape.
Popup {
    id: control

    readonly property Rectangle background: bg

    property rgba textColor
    property rgba titleColor: textColor
    property string title: ""
    property string message: ""
    property string acceptText: qsTr("OK")
    property string rejectText: qsTr("Cancel")
    property bool showRejectButton: true

    modal: true
    dimBackground: true
    closePolicy: Popup.CloseOnEscape
    implicitWidth: Math.max(32, contentColumn.implicitWidth + 4)
    implicitHeight: contentColumn.implicitHeight + 2
    initialFocusItem: acceptButton
    anchors.centerIn: parent

    Rectangle {
        id: bg
        anchors.fill: parent
        color: QmlTui.defaultColor
        border.style: Border.Double
    }

    Keys.onPressed: event => {
        if ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && !rejectButton.activeFocus) {
            acceptButton.clicked();
            event.accepted = true;
        }
    }

    Column {
        id: contentColumn
        x: 2
        y: 1
        spacing: 1

        Text {
            text: control.title
            bold: true
            color: control.titleColor
        }

        Text {
            text: control.message
            color: control.textColor
        }

        Row {
            spacing: 2

            Button {
                id: acceptButton
                text: control.acceptText
                onClicked: control.accept()
            }

            Button {
                id: rejectButton
                visible: control.showRejectButton
                text: control.rejectText
                onClicked: control.reject()
            }
        }
    }
}
