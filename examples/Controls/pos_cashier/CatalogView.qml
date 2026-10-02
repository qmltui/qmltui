import QtQuick
import Qtui
import Qtui.Controls

Item {
    id: root
    property var products: []
    property string currentCategory: "ALL"
    property var cartItems: []
    property double totalAmount: 0.0
    property double subtotal: 0.0
    property double taxAmount: 0.0
    property double discountAmount: 0.0
    property double discountRate: 0.0
    property int itemCount: 0

    signal addItemRequested(var product)
    signal checkoutRequested()
    signal viewCartRequested()
    signal categorySelected(string category)

    // Left Side: Category Bar & Product Catalog Grid (68% width)
    Item {
        id: catalogLeft
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: Math.floor(parent.width * 0.68)

        Panel {
            id: categoryPanel
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 5
            color: "#0d1c2d"
            border.color: "#273647"
            title: qsTr("🏷️ CATEGORIES")
            titleColor: "#80DEEA"

            Row {
                x: 1
                y: 1
                spacing: 1

                Button {
                    label: root.currentCategory === "ALL" ? "[ ALL ]" : " ALL "
                    normalColor: root.currentCategory === "ALL" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "ALL" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: root.currentCategory === "ALL" ? "double" : "single"
                    onClicked: root.categorySelected("ALL")
                }
                Button {
                    label: root.currentCategory === "COFFEE" ? "[ COFFEE ]" : " COFFEE "
                    normalColor: root.currentCategory === "COFFEE" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "COFFEE" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: root.currentCategory === "COFFEE" ? "double" : "single"
                    onClicked: root.categorySelected("COFFEE")
                }
                Button {
                    label: root.currentCategory === "PASTRIES" ? "[ PASTRIES ]" : " PASTRIES "
                    normalColor: root.currentCategory === "PASTRIES" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "PASTRIES" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: root.currentCategory === "PASTRIES" ? "double" : "single"
                    onClicked: root.categorySelected("PASTRIES")
                }
                Button {
                    label: root.currentCategory === "TEA" ? "[ TEA ]" : " TEA "
                    normalColor: root.currentCategory === "TEA" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "TEA" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: root.currentCategory === "TEA" ? "double" : "single"
                    onClicked: root.categorySelected("TEA")
                }
                Button {
                    label: root.currentCategory === "COLD DRINKS" ? "[ COLD DRINKS ]" : " COLD DRINKS "
                    normalColor: root.currentCategory === "COLD DRINKS" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "COLD DRINKS" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: root.currentCategory === "COLD DRINKS" ? "double" : "single"
                    onClicked: root.categorySelected("COLD DRINKS")
                }
                Button {
                    label: root.currentCategory === "MERCHANDISE" ? "[ MERCH ]" : " MERCH "
                    normalColor: root.currentCategory === "MERCHANDISE" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "MERCHANDISE" ? "#051424" : "#D4E4FA"
                    normalBorderStyle: root.currentCategory === "MERCHANDISE" ? "double" : "single"
                    onClicked: root.categorySelected("MERCHANDISE")
                }
            }
        }

        Panel {
            id: catalogGridPanel
            anchors.top: categoryPanel.bottom
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            color: "#051424"
            border.color: "#273647"
            title: qsTr("📦 PRODUCT CATALOG (%1)").arg(root.currentCategory)
            titleColor: "#4AE183"

            Column {
                x: 1
                y: 1
                width: parent.width - 2
                spacing: 0

                // Grid header
                Row {
                    width: parent.width
                    spacing: 1
                    Text { width: Math.floor(parent.width * 0.12); text: "SKU"; color: "#80DEEA"; bold: true }
                    Text { width: Math.floor(parent.width * 0.38); text: "PRODUCT NAME"; color: "#80DEEA"; bold: true }
                    Text { width: Math.floor(parent.width * 0.18); text: "CATEGORY"; color: "#80DEEA"; bold: true }
                    Text { width: Math.floor(parent.width * 0.14); text: "PRICE"; color: "#80DEEA"; bold: true }
                    Text { width: Math.floor(parent.width * 0.15); text: "ACTION"; color: "#80DEEA"; bold: true }
                }

                Text {
                    width: parent.width
                    text: "────────────────────────────────────────────────────────────────────────────"
                    color: "#273647"
                }

                // Product items list
                Repeater {
                    model: root.products

                    Row {
                        id: prodRow
                        property var modelData: null
                        visible: (root.currentCategory === "ALL" || (modelData && modelData.category === root.currentCategory))
                        width: parent.width
                        spacing: 1

                        Text { width: Math.floor(parent.width * 0.12); text: prodRow.modelData ? prodRow.modelData.sku : ""; color: "#8F9094" }
                        Text { width: Math.floor(parent.width * 0.38); text: prodRow.modelData ? prodRow.modelData.name + " (" + prodRow.modelData.detail + ")" : ""; color: "#FFFFFF"; bold: true }
                        Text { width: Math.floor(parent.width * 0.18); text: prodRow.modelData ? prodRow.modelData.category : ""; color: "#FFB961" }
                        Text { width: Math.floor(parent.width * 0.14); text: prodRow.modelData ? "$" + prodRow.modelData.price.toFixed(2) : ""; color: "#4AE183"; bold: true }
                        Button {
                            label: "+ ADD"
                            normalColor: "#1c2b3c"
                            textColor: "#4AE183"
                            normalBorderStyle: "none"
                            focusBorderStyle: "single"
                            onClicked: {
                                if (prodRow.modelData) {
                                    root.addItemRequested(prodRow.modelData);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Right Side: Cart Summary & Quick Action Panel (32% width)
    Item {
        id: catalogRight
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: catalogLeft.right
        anchors.right: parent.right

        Panel {
            id: quickCartPanel
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "#0d1c2d"
            border.color: "#273647"
            title: qsTr("🛒 CURRENT CART (%1)").arg(root.itemCount)
            titleColor: "#4AE183"

            Column {
                x: 1
                y: 1
                width: parent.width - 2
                spacing: 1

                Text {
                    text: qsTr("TOTAL: $%1").arg(root.totalAmount.toFixed(2))
                    color: "#4AE183"
                    bold: true
                }
                Text {
                    text: qsTr("SUBTOTAL: $%1").arg(root.subtotal.toFixed(2))
                    color: "#D4E4FA"
                }
                Text {
                    text: qsTr("TAX (8.5%): $%1").arg(root.taxAmount.toFixed(2))
                    color: "#8F9094"
                }
                Text {
                    visible: root.discountRate > 0
                    text: qsTr("DISCOUNT: -$%1").arg(root.discountAmount.toFixed(2))
                    color: "#F48FB1"
                }

                Text { text: "─────────────────────────────────"; color: "#273647" }

                Row {
                    spacing: 1
                    Button {
                        label: "[F12] PAY NOW"
                        normalColor: "#06BB63"
                        textColor: "#051424"
                        normalBorderStyle: "single"
                        onClicked: root.checkoutRequested()
                    }
                    Button {
                        label: "[F4] VIEW CART"
                        normalColor: "#1c2b3c"
                        textColor: "#D4E4FA"
                        normalBorderStyle: "single"
                        onClicked: root.viewCartRequested()
                    }
                }

                Text { text: ""; height: 1 }
                Text { text: qsTr("QUICK CART ITEMS:"); color: "#80DEEA"; bold: true }

                Repeater {
                    model: root.cartItems

                    Row {
                        id: qRow
                        property var modelData: null
                        width: parent.width
                        spacing: 1

                        Text { width: Math.floor(parent.width * 0.50); text: qRow.modelData ? qRow.modelData.name : ""; color: "#FFFFFF" }
                        Text { width: Math.floor(parent.width * 0.18); text: qRow.modelData ? "x" + qRow.modelData.qty : ""; color: "#FFB961" }
                        Text { width: Math.floor(parent.width * 0.28); text: qRow.modelData ? "$" + (qRow.modelData.qty * qRow.modelData.price).toFixed(2) : ""; color: "#4AE183" }
                    }
                }

                Text {
                    visible: root.cartItems.length === 0
                    text: qsTr("(Cart is empty)")
                    color: "#8F9094"
                }
            }
        }
    }
}
