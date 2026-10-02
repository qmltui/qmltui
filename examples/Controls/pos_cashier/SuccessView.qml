import QtQuick
import Qtui
import Qtui.Controls

Item {
    id: root
    property string transactionId: ""
    property double paidAmount: 0.0
    property string paymentMethod: ""
    property string authCode: ""

    signal receiptOptionClicked(string option)
    signal newSaleRequested()

    Panel {
        anchors.centerIn: parent
        width: Math.min(64, Math.max(44, parent.width - 4))
        height: 17
        color: "#0d1c2d"
        border.color: "#4AE183"
        title: qsTr("✓ PAYMENT SUCCESSFUL")
        titleColor: "#4AE183"

        Column {
            x: 1
            y: 1
            width: parent.width - 2
            spacing: 1

            Text {
                text: qsTr("🎉 TRANSACTION COMPLETED")
                color: "#4AE183"
                bold: true
            }
            Text {
                text: qsTr("TRANSACTION ID: #%1").arg(root.transactionId)
                color: "#8F9094"
            }
            Text {
                text: qsTr("TOTAL PAID: $%1").arg(root.paidAmount.toFixed(2))
                color: "#4AE183"
                bold: true
            }
            Text {
                text: qsTr("METHOD: %1  |  AUTH CODE: %2").arg(root.paymentMethod).arg(root.authCode)
                color: "#D4E4FA"
            }

            Text { text: "──────────────────────────────────────────────────"; color: "#273647" }
            Text { text: qsTr("RECEIPT OPTIONS:"); color: "#80DEEA"; bold: true }

            Row {
                spacing: 1
                Button {
                    label: "🖨️ PRINT RECEIPT"
                    normalColor: "#1c2b3c"
                    textColor: "#D4E4FA"
                    normalBorderStyle: "single"
                    onClicked: root.receiptOptionClicked("PRINT")
                }
                Button {
                    label: "✉️ EMAIL RECEIPT"
                    normalColor: "#1c2b3c"
                    textColor: "#D4E4FA"
                    normalBorderStyle: "single"
                    onClicked: root.receiptOptionClicked("EMAIL")
                }
                Button {
                    label: "💬 SMS RECEIPT"
                    normalColor: "#1c2b3c"
                    textColor: "#D4E4FA"
                    normalBorderStyle: "single"
                    onClicked: root.receiptOptionClicked("SMS")
                }
            }

            Text { text: "──────────────────────────────────────────────────"; color: "#273647" }

            Button {
                label: "[ + ] START NEW SALE"
                normalColor: "#06BB63"
                textColor: "#051424"
                normalBorderStyle: "single"
                onClicked: root.newSaleRequested()
            }
        }
    }
}
