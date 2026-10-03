import QtQuick
import Qtui
import Qtui.Controls

Item {
    id: root
    property double totalAmount: 0.0
    property int itemCount: 0
    property string paymentMethod: "CASH"
    property string cashTenderedText: ""

    signal paymentMethodSelected(string method)
    signal cashTenderedChanged(string text)
    signal confirmPaymentRequested()
    signal backToCartRequested()

    Panel {
        anchors.centerIn: parent
        width: Math.min(64, Math.max(42, parent.width - 4))
        height: 16
        color: "#0d1c2d"
        border.color: "#4AE183"
        title: qsTr("💳 PAYMENT METHOD SELECTION")
        titleColor: "#4AE183"

        Column {
            x: 1
            y: 1
            width: parent.width - 2
            spacing: 1

            Text {
                text: qsTr("TOTAL AMOUNT DUE: $%1").arg(root.totalAmount.toFixed(2))
                color: "#4AE183"
                bold: true
            }
            Text {
                text: qsTr("ITEMS IN CART: %1").arg(root.itemCount)
                color: "#D4E4FA"
            }

            Text { text: "──────────────────────────────────────────────────"; color: "#273647" }
            Text { text: qsTr("SELECT METHOD:"); color: "#80DEEA"; bold: true }

            // Bento style payment option grid
            Row {
                spacing: 1
                Button {
                    label: root.paymentMethod === "CASH" ? "[✓ 💵 CASH]" : "  💵 CASH  "
                    normalColor: root.paymentMethod === "CASH" ? "#06BB63" : "#1c2b3c"
                    textColor: root.paymentMethod === "CASH" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: "single"
                    onClicked: root.paymentMethodSelected("CASH")
                }
                Button {
                    label: root.paymentMethod === "CARD" ? "[✓ 💳 CREDIT CARD]" : " 💳 CARD  "
                    normalColor: root.paymentMethod === "CARD" ? "#06BB63" : "#1c2b3c"
                    textColor: root.paymentMethod === "CARD" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: "single"
                    onClicked: root.paymentMethodSelected("CARD")
                }
            }
            Row {
                spacing: 1
                Button {
                    label: root.paymentMethod === "MOBILE" ? "[✓ 📱 MOBILE PAY]" : " 📱 MOBILE "
                    normalColor: root.paymentMethod === "MOBILE" ? "#06BB63" : "#1c2b3c"
                    textColor: root.paymentMethod === "MOBILE" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: "single"
                    onClicked: root.paymentMethodSelected("MOBILE")
                }
                Button {
                    label: root.paymentMethod === "SPLIT" ? "[✓ 🔀 SPLIT PAY]" : " 🔀 SPLIT  "
                    normalColor: root.paymentMethod === "SPLIT" ? "#06BB63" : "#1c2b3c"
                    textColor: root.paymentMethod === "SPLIT" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: "single"
                    onClicked: root.paymentMethodSelected("SPLIT")
                }
            }

            Text { text: "──────────────────────────────────────────────────"; color: "#273647" }

            Row {
                visible: root.paymentMethod === "CASH"
                spacing: 1
                Text { text: qsTr("CASH TENDERED: $"); color: "#FFB961" }
                TextInput {
                    width: 10
                    text: root.cashTenderedText
                    color: "#4AE183"
                    onTextChanged: root.cashTenderedChanged(text)
                }
            }

            Text {
                visible: root.paymentMethod === "CASH"
                text: qsTr("CHANGE DUE: $%1").arg(Math.max(0.0, (parseFloat(root.cashTenderedText) || 0.0) - root.totalAmount).toFixed(2))
                color: "#4AE183"
                bold: true
            }

            Text { text: ""; height: 1 }

            Row {
                spacing: 2
                Button {
                    label: "[ENTER] CONFIRM PAYMENT"
                    normalColor: "#06BB63"
                    textColor: "#051424"
                    normalBorderStyle: "single"
                    onClicked: root.confirmPaymentRequested()
                }
                Button {
                    label: "[ESC] BACK TO CART"
                    normalColor: "#1c2b3c"
                    textColor: "#D4E4FA"
                    normalBorderStyle: "single"
                    onClicked: root.backToCartRequested()
                }
            }
        }
    }
}
