import QtQml
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
        width: Math.min(66, Math.max(46, parent.width - 2))
        height: Math.min(17, parent.height)
        color: "#0d1c2d"
        border.color: "#4AE183"
        title: qsTr("💳 PAYMENT METHOD SELECTION")
        titleColor: "#4AE183"

        Column {
            x: 1
            y: 1
            width: parent.width - 2
            spacing: 0

            Row {
                spacing: 3
                Text {
                    text: qsTr("TOTAL DUE: $%1").arg(root.totalAmount.toFixed(2))
                    color: "#4AE183"
                    bold: true
                }
                Text {
                    text: qsTr("ITEMS IN CART: %1").arg(root.itemCount)
                    color: "#D4E4FA"
                }
            }

            Text { text: "──────────────────────────────────────────────────────────────"; color: "#273647" }
            Text { text: qsTr("SELECT METHOD:"); color: "#80DEEA"; bold: true }

            Row {
                spacing: 1
                Button {
                    background: Rectangle {
                        color: root.paymentMethod === "CASH" ? "#06BB63" : "#1c2b3c"
                        border.color: "default"
                        border.style: "single"
                    }
                    text: root.paymentMethod === "CASH" ? "[✓ 💵 CASH]" : " 💵 CASH "
                    textColor: root.paymentMethod === "CASH" ? "#051424" : "#D4E4FA"
                    onClicked: root.paymentMethodSelected("CASH")
                }
                Button {
                    background: Rectangle {
                        color: root.paymentMethod === "CARD" ? "#06BB63" : "#1c2b3c"
                        border.color: "default"
                        border.style: "single"
                    }
                    text: root.paymentMethod === "CARD" ? "[✓ 💳 CARD]" : " 💳 CARD "
                    textColor: root.paymentMethod === "CARD" ? "#051424" : "#D4E4FA"
                    onClicked: root.paymentMethodSelected("CARD")
                }
                Button {
                    background: Rectangle {
                        color: root.paymentMethod === "MOBILE" ? "#06BB63" : "#1c2b3c"
                        border.color: "default"
                        border.style: "single"
                    }
                    text: root.paymentMethod === "MOBILE" ? "[✓ 📱 MOBILE]" : " 📱 MOBILE "
                    textColor: root.paymentMethod === "MOBILE" ? "#051424" : "#D4E4FA"
                    onClicked: root.paymentMethodSelected("MOBILE")
                }
                Button {
                    background: Rectangle {
                        color: root.paymentMethod === "SPLIT" ? "#06BB63" : "#1c2b3c"
                        border.color: "default"
                        border.style: "single"
                    }
                    text: root.paymentMethod === "SPLIT" ? "[✓ 🔀 SPLIT]" : " 🔀 SPLIT "
                    textColor: root.paymentMethod === "SPLIT" ? "#051424" : "#D4E4FA"
                    onClicked: root.paymentMethodSelected("SPLIT")
                }
            }

            Text { text: "──────────────────────────────────────────────────────────────"; color: "#273647" }

            Column {
                visible: root.paymentMethod === "CASH"
                spacing: 0

                Row {
                    spacing: 1
                    Text { text: qsTr("CASH TENDERED: $"); color: "#FFB961" }
                    TextInput {
                        width: 8
                        text: root.cashTenderedText
                        color: "#4AE183"
                        onTextChanged: root.cashTenderedChanged(text)
                    }
                    Text { text: "  QUICK: "; color: "#80DEEA" }
                    Button {
                        background: Rectangle {
                            color: "#1c2b3c"
                            border.color: "default"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? "double" : "single") : "none"
                        }
                        text: "[Exact]"
                        textColor: "#D4E4FA"
                        onClicked: root.cashTenderedChanged(root.totalAmount.toFixed(2))
                    }
                    Button {
                        background: Rectangle {
                            color: "#1c2b3c"
                            border.color: "default"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? "double" : "single") : "none"
                        }
                        text: "[$20]"
                        textColor: "#D4E4FA"
                        onClicked: root.cashTenderedChanged("20.00")
                    }
                    Button {
                        background: Rectangle {
                            color: "#1c2b3c"
                            border.color: "default"
                            border.style: parent && parent.height >= 3 ? (parent.activeFocus ? "double" : "single") : "none"
                        }
                        text: "[$50]"
                        textColor: "#D4E4FA"
                        onClicked: root.cashTenderedChanged("50.00")
                    }
                }
                Text {
                    property double tenderedVal: parseFloat(root.cashTenderedText) || 0.0
                    text: tenderedVal < root.totalAmount ? qsTr("⚠️ SHORTFALL: -$%1").arg((root.totalAmount - tenderedVal).toFixed(2)) : qsTr("CHANGE DUE: $%1").arg((tenderedVal - root.totalAmount).toFixed(2))
                    color: (parseFloat(root.cashTenderedText) || 0.0) < root.totalAmount ? "#FF5555" : "#4AE183"
                    bold: true
                }
            }

            Column {
                visible: root.paymentMethod === "CARD"
                spacing: 0
                Text { text: qsTr("CARD TERMINAL: READY"); color: "#4AE183"; bold: true }
                Text { text: qsTr("Insert, swipe, or tap card on terminal reader."); color: "#D4E4FA" }
            }

            Column {
                visible: root.paymentMethod === "MOBILE"
                spacing: 0
                Text { text: qsTr("NFC SCANNER: ACTIVE"); color: "#4AE183"; bold: true }
                Text { text: qsTr("Hold Apple Pay or Google Pay device near reader."); color: "#D4E4FA" }
            }

            Column {
                visible: root.paymentMethod === "SPLIT"
                spacing: 0
                Text { text: qsTr("SPLIT PAYMENT: 50% CASH / 50% CARD"); color: "#FFB961"; bold: true }
                Text { text: qsTr("Part 1: $%1 (Cash) | Part 2: $%2 (Card)").arg((root.totalAmount / 2.0).toFixed(2)).arg((root.totalAmount / 2.0).toFixed(2)); color: "#D4E4FA" }
            }

            Text { text: "──────────────────────────────────────────────────────────────"; color: "#273647" }

            Row {
                spacing: 2
                Button {
                    background: Rectangle {
                        color: "#1c2b3c"
                        border.color: "default"
                        border.style: "single"
                    }
                    text: "[ESC] BACK TO CART"
                    textColor: "#D4E4FA"
                    onClicked: root.backToCartRequested()
                }
                Button {
                    background: Rectangle {
                        color: "#06BB63"
                        border.color: "default"
                        border.style: "single"
                    }
                    text: "[ENTER] CONFIRM PAYMENT"
                    textColor: "#051424"
                    onClicked: root.confirmPaymentRequested()
                }
            }
        }
    }
}
