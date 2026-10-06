pragma ComponentBehavior: Bound

import QtQml
import Qtui
import Qtui.Controls

Item {
    id: root
    property var products: []
    property var cartItems: []
    property string currentCategory: "ALL"
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

    // Left Side: Catalog & Product Grid (64% width)
    Item {
        id: catalogLeft
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: Math.floor(parent.width * 0.64)

        // Category Tabs Panel
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
                    text: root.currentCategory === "ALL" ? "[ ALL ]" : "  ALL  "
                    color: root.currentCategory === "ALL" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "ALL" ? "#051424" : "#D4E4FA"
                    border.style: "single"
                    onClicked: root.categorySelected("ALL")
                }
                Button {
                    text: root.currentCategory === "COFFEE" ? "[ COFFEE ]" : " COFFEE "
                    color: root.currentCategory === "COFFEE" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "COFFEE" ? "#051424" : "#D4E4FA"
                    border.style: "single"
                    onClicked: root.categorySelected("COFFEE")
                }
                Button {
                    text: root.currentCategory === "PASTRIES" ? "[ PASTRIES ]" : " PASTRIES "
                    color: root.currentCategory === "PASTRIES" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "PASTRIES" ? "#051424" : "#D4E4FA"
                    border.style: "single"
                    onClicked: root.categorySelected("PASTRIES")
                }
                Button {
                    text: root.currentCategory === "TEA" ? "[ TEA ]" : " TEA "
                    color: root.currentCategory === "TEA" ? "#06BB63" : "#1c2b3c"
                    textColor: root.currentCategory === "TEA" ? "#051424" : "#D4E4FA"
                    border.style: "single"
                    onClicked: root.categorySelected("TEA")
                }
            }
        }

        // Product Catalog Grid Panel
        Panel {
            id: productPanel
            anchors.top: categoryPanel.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "#051424"
            border.color: "#273647"
            title: qsTr("📦 PRODUCT CATALOG (%1)").arg(root.currentCategory)
            titleColor: "#4AE183"

            Column {
                x: 1
                y: 1
                width: parent ? parent.width - 2 : 0
                spacing: 0

                // Grid header
                Row {
                    width: parent ? parent.width : 0
                    spacing: 1
                    Text { width: parent ? Math.floor(parent.width * 0.10) : 0; text: "SKU"; color: "#80DEEA"; bold: true }
                    Text { width: parent ? Math.floor(parent.width * 0.36) : 0; text: "PRODUCT NAME"; color: "#80DEEA"; bold: true }
                    Text { width: parent ? Math.floor(parent.width * 0.16) : 0; text: "CATEGORY"; color: "#80DEEA"; bold: true }
                    Text { width: parent ? Math.floor(parent.width * 0.14) : 0; text: "PRICE"; color: "#80DEEA"; bold: true }
                    Text { width: parent ? Math.floor(parent.width * 0.22) : 0; text: "ACTION"; color: "#80DEEA"; bold: true }
                }

                Text {
                    width: parent ? parent.width : 0
                    text: "────────────────────────────────────────────────────────────────────────────"
                    color: "#273647"
                }

                // Product items list (bounded to 6 items to fit inside workspace)
                Repeater {
                    model: root.products.filter(p => root.currentCategory === "ALL" || p.category === root.currentCategory).slice(0, 6)

                    Row {
                        id: prodRow
                        required property var modelData
                        width: parent ? parent.width : 0
                        spacing: 1

                        Text { width: parent ? Math.floor(parent.width * 0.10) : 0; text: prodRow.modelData ? prodRow.modelData.sku : ""; color: "#8F9094" }
                        Text { width: parent ? Math.floor(parent.width * 0.36) : 0; text: prodRow.modelData ? prodRow.modelData.name : ""; color: "#FFFFFF"; bold: true }
                        Text { width: parent ? Math.floor(parent.width * 0.16) : 0; text: prodRow.modelData ? prodRow.modelData.category : ""; color: "#FFB961" }
                        Text { width: parent ? Math.floor(parent.width * 0.14) : 0; text: prodRow.modelData ? "$" + prodRow.modelData.price.toFixed(2) : ""; color: "#4AE183"; bold: true }
                        Button {
                            height: 1
                            width: parent ? Math.floor(parent.width * 0.22) : 0
                            text: "+ ADD"
                            color: "#1c2b3c"
                            textColor: "#4AE183"
                            border.style: "none"
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

    // Right Side: Cart Summary & Quick Action Panel (36% width)
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
            height: Math.min(15, parent.height)
            color: "#0d1c2d"
            border.color: "#273647"
            title: qsTr("🛒 CURRENT CART (%1)").arg(root.itemCount)
            titleColor: "#4AE183"

            Column {
                x: 1
                y: 1
                width: parent ? parent.width - 2 : 0
                spacing: 0

                Text {
                    text: qsTr("TOTAL: $%1").arg(root.totalAmount.toFixed(2))
                    color: "#4AE183"
                    bold: true
                }
                Text {
                    text: qsTr("SUBTOTAL: $%1 | TAX: $%2").arg(root.subtotal.toFixed(2)).arg(root.taxAmount.toFixed(2))
                    color: "#D4E4FA"
                }

                Text { text: "─────────────────────────────────"; color: "#273647" }

                Row {
                    spacing: 1
                    Button {
                        text: "[F12] PAY NOW"
                        color: "#06BB63"
                        textColor: "#051424"
                        border.style: "single"
                        onClicked: root.checkoutRequested()
                    }
                    Button {
                        text: "[F4] VIEW CART"
                        color: "#1c2b3c"
                        textColor: "#D4E4FA"
                        border.style: "single"
                        onClicked: root.viewCartRequested()
                    }
                }

                Text { text: "─────────────────────────────────"; color: "#273647" }
                Text { text: qsTr("QUICK CART ITEMS:"); color: "#80DEEA"; bold: true }

                Repeater {
                    model: root.cartItems

                    Row {
                        id: qRow
                        required property var modelData
                        width: parent ? parent.width : 0
                        spacing: 1

                        Text { width: parent ? Math.floor(parent.width * 0.50) : 0; text: qRow.modelData ? qRow.modelData.name : ""; color: "#FFFFFF" }
                        Text { width: parent ? Math.floor(parent.width * 0.18) : 0; text: qRow.modelData ? "x" + qRow.modelData.qty : ""; color: "#FFB961" }
                        Text { width: parent ? Math.floor(parent.width * 0.28) : 0; text: qRow.modelData ? "$" + (qRow.modelData.qty * qRow.modelData.price).toFixed(2) : ""; color: "#4AE183" }
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
