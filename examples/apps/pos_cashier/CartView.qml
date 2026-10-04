import QtQuick
import Qtui
import Qtui.Controls

Item {
    id: root
    property var cartItems: []
    property double totalAmount: 0.0
    property double subtotal: 0.0
    property double taxAmount: 0.0
    property double discountRate: 0.0
    property int itemCount: 0
    property string keypadBuffer: "0.00"

    signal updateQtyRequested(string sku, int delta)
    signal clearCartRequested()
    signal applyDiscountRequested(double rate)
    signal checkoutRequested()
    signal keypadKeyPressed(string key)
    signal customItemRequested()

    // Left Side: Full Cart Details (60% width)
    Item {
        id: cartLeft
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: Math.floor(parent.width * 0.60)

        Panel {
            id: fullCartPanel
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "#051424"
            border.color: "#273647"
            title: qsTr("🛒 CART ITEMS (%1 ITEMS)").arg(root.itemCount)
            titleColor: "#4AE183"

            Column {
                x: 1
                y: 1
                width: parent ? parent.width - 2 : 0
                spacing: 0

                Row {
                    width: parent ? parent.width : 0
                    spacing: 1
                    Text { width: parent ? Math.floor(parent.width * 0.30) : 0; text: "ITEM NAME"; color: "#80DEEA"; bold: true }
                    Text { width: parent ? Math.floor(parent.width * 0.14) : 0; text: "PRICE"; color: "#80DEEA"; bold: true }
                    Text { width: parent ? Math.floor(parent.width * 0.34) : 0; text: "QTY ADJUST"; color: "#80DEEA"; bold: true }
                    Text { width: parent ? Math.floor(parent.width * 0.18) : 0; text: "TOTAL"; color: "#80DEEA"; bold: true }
                }

                Text { text: "────────────────────────────────────────────────────────────"; color: "#273647" }

                Repeater {
                    model: root.cartItems

                    Row {
                        id: cRow
                        property var modelData: null
                        width: parent ? parent.width : 0
                        spacing: 1

                        Text { width: parent ? Math.floor(parent.width * 0.30) : 0; text: cRow.modelData ? cRow.modelData.name : ""; color: "#FFFFFF"; bold: true }
                        Text { width: parent ? Math.floor(parent.width * 0.14) : 0; text: cRow.modelData ? "$" + cRow.modelData.price.toFixed(2) : ""; color: "#D4E4FA" }

                        Row {
                            width: parent ? Math.floor(parent.width * 0.34) : 0
                            spacing: 1
                            Button {
                                label: "-"
                                normalColor: "#1c2b3c"
                                textColor: "#FFB961"
                                normalBorderStyle: "none"
                                focusBorderStyle: "single"
                                onClicked: {
                                    if (cRow.modelData) root.updateQtyRequested(cRow.modelData.sku, -1);
                                }
                            }
                            Text {
                                text: cRow.modelData ? " " + cRow.modelData.qty + " " : " 0 "
                                color: "#FFB961"
                                bold: true
                            }
                            Button {
                                label: "+"
                                normalColor: "#1c2b3c"
                                textColor: "#4AE183"
                                normalBorderStyle: "none"
                                focusBorderStyle: "single"
                                onClicked: {
                                    if (cRow.modelData) root.updateQtyRequested(cRow.modelData.sku, 1);
                                }
                            }
                        }

                        Text { width: parent ? Math.floor(parent.width * 0.18) : 0; text: cRow.modelData ? "$" + (cRow.modelData.qty * cRow.modelData.price).toFixed(2) : ""; color: "#4AE183"; bold: true }
                    }
                }

                Text {
                    visible: root.cartItems.length === 0
                    text: qsTr("Cart is empty! Click [F3 CATALOG] to add items.")
                    color: "#8F9094"
                }

                Text { text: "────────────────────────────────────────────────────────────"; color: "#273647" }

                Row {
                    spacing: 2
                    Button {
                        label: "[F1] CLEAR SALE"
                        normalColor: "#800000"
                        textColor: "#FFFFFF"
                        normalBorderStyle: "single"
                        onClicked: root.clearCartRequested()
                    }
                    Button {
                        label: root.discountRate > 0 ? "[✓ 10% DISC]" : "[F2] 10% DISC"
                        normalColor: root.discountRate > 0 ? "#FFB961" : "#1c2b3c"
                        textColor: root.discountRate > 0 ? "#051424" : "#D4E4FA"
                        normalBorderStyle: "single"
                        onClicked: root.applyDiscountRequested(root.discountRate > 0 ? 0.0 : 0.10)
                    }
                }
            }
        }
    }

    // Right Side: Keypad & Quick Amount Entry Panel (40% width)
    Item {
        id: cartRight
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: cartLeft.right
        anchors.right: parent.right

        Keypad {
            anchors.fill: parent
            buffer: root.keypadBuffer
            totalAmount: root.totalAmount
            subtotal: root.subtotal
            taxAmount: root.taxAmount

            onKeyPressed: (key) => root.keypadKeyPressed(key)
            onCustomItemRequested: root.customItemRequested()
            onCheckoutRequested: root.checkoutRequested()
        }
    }
}
