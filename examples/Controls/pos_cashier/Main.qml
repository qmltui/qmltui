import QtQuick
import Qtui
import Qtui.Controls
import Qtui.Widgets

Terminal {
    id: rootWindow
    useAlternateScreen: true
    focus: true
    color: "#051424"
    state: "catalog"

    // --- State Properties ---
    property string storeName: "RapidPOS #04"
    property string cashierName: "Alex M."
    property double taxRate: 0.085 // 8.5% Tax
    property double discountRate: 0.0 // Discount fraction
    property string currentCategory: "ALL"
    property string statusNotification: ""

    // Keypad entry buffer
    property string keypadBuffer: "0.00"

    // Payment state
    property string paymentMethod: "CASH" // "CASH", "CARD", "MOBILE", "SPLIT"
    property string cashTenderedText: ""
    property string lastTransactionId: "POS-2026-9942"
    property string lastAuthCode: "XP-9921"
    property double lastPaidAmount: 0.0
    property string lastPaymentMethod: "CASH"

    // Products Database Model
    property var products: [
        { id: "P01", sku: "10492", name: "Classic Latte", category: "COFFEE", price: 4.50, detail: "12 OZ" },
        { id: "P02", sku: "08123", name: "Butter Croissant", category: "PASTRIES", price: 3.75, detail: "BAKERY" },
        { id: "P03", sku: "22019", name: "Nitro Cold Brew", category: "COFFEE", price: 5.25, detail: "16 OZ" },
        { id: "P04", sku: "33012", name: "Honeycrisp Apples", category: "MERCHANDISE", price: 2.99, detail: "1 KG" },
        { id: "P05", sku: "55011", name: "Ribeye Steak", category: "MERCHANDISE", price: 16.50, detail: "FRESH" },
        { id: "P06", sku: "44081", name: "Fresh OJ 1L", category: "COLD DRINKS", price: 3.25, detail: "BOTTLE" },
        { id: "P07", sku: "10501", name: "Artisanal Espresso", category: "COFFEE", price: 4.50, detail: "DOUBLE" },
        { id: "P08", sku: "08125", name: "Almond Croissant", category: "PASTRIES", price: 5.25, detail: "BAKERY" },
        { id: "P09", sku: "66010", name: "Travel Mug (L)", category: "MERCHANDISE", price: 24.00, detail: "MATTE BLK" },
        { id: "P10", sku: "22025", name: "Matcha Latte", category: "TEA", price: 6.20, detail: "12 OZ" },
        { id: "P11", sku: "77002", name: "Iced Green Tea", category: "TEA", price: 3.50, detail: "16 OZ" },
        { id: "P12", sku: "08130", name: "Sourdough Bread", category: "PASTRIES", price: 3.80, detail: "LOAF" }
    ]

    // Cart Items Model
    property var cartItems: [
        { id: "P01", sku: "10492", name: "Classic Latte", price: 4.50, qty: 2 },
        { id: "P02", sku: "08123", name: "Butter Croissant", price: 3.75, qty: 1 },
        { id: "P03", sku: "22019", name: "Nitro Cold Brew", price: 5.25, qty: 1 }
    ]

    // Declarative QML States for Screen Management
    states: [
        State {
            name: "catalog"
            PropertyChanges { target: catalogView; properties: { "visible": true } }
            PropertyChanges { target: cartView; properties: { "visible": false } }
            PropertyChanges { target: paymentView; properties: { "visible": false } }
            PropertyChanges { target: successView; properties: { "visible": false } }
        },
        State {
            name: "cart"
            PropertyChanges { target: catalogView; properties: { "visible": false } }
            PropertyChanges { target: cartView; properties: { "visible": true } }
            PropertyChanges { target: paymentView; properties: { "visible": false } }
            PropertyChanges { target: successView; properties: { "visible": false } }
        },
        State {
            name: "payment"
            PropertyChanges { target: catalogView; properties: { "visible": false } }
            PropertyChanges { target: cartView; properties: { "visible": false } }
            PropertyChanges { target: paymentView; properties: { "visible": true } }
            PropertyChanges { target: successView; properties: { "visible": false } }
        },
        State {
            name: "receipt"
            PropertyChanges { target: catalogView; properties: { "visible": false } }
            PropertyChanges { target: cartView; properties: { "visible": false } }
            PropertyChanges { target: paymentView; properties: { "visible": false } }
            PropertyChanges { target: successView; properties: { "visible": true } }
        }
    ]

    transitions: [
        Transition {
            from: "*"
            to: "*"
        }
    ]

    // Helper functions
    function getSubtotal() {
        let sum = 0.0;
        for (let i = 0; i < cartItems.length; i++) sum += cartItems[i].qty * cartItems[i].price;
        return sum;
    }

    function getTaxAmount() { return getSubtotal() * taxRate; }
    function getDiscountAmount() { return getSubtotal() * discountRate; }
    function getTotalAmount() { return Math.max(0.0, getSubtotal() + getTaxAmount() - getDiscountAmount()); }
    function getItemCount() {
        let count = 0;
        for (let i = 0; i < cartItems.length; i++) count += cartItems[i].qty;
        return count;
    }

    function addItem(prod) {
        let items = Array.from(cartItems);
        let found = false;
        for (let i = 0; i < items.length; i++) {
            if (items[i].id === prod.id || items[i].sku === prod.sku) {
                items[i].qty += 1;
                found = true;
                break;
            }
        }
        if (!found) items.push({ id: prod.id, sku: prod.sku, name: prod.name, price: prod.price, qty: 1 });
        cartItems = items;
        statusNotification = qsTr("Added %1 to cart").arg(prod.name);
    }

    function updateQty(sku, delta) {
        let items = Array.from(cartItems);
        for (let i = 0; i < items.length; i++) {
            if (items[i].sku === sku) {
                items[i].qty += delta;
                if (items[i].qty <= 0) items.splice(i, 1);
                break;
            }
        }
        cartItems = items;
    }

    function clearCart() { cartItems = []; statusNotification = qsTr("Cart cleared"); }
    function applyDiscount(rate) { discountRate = rate; statusNotification = rate > 0 ? qsTr("Applied %1% discount").arg(Math.round(rate * 100)) : qsTr("Discount removed"); }

    function handleKeypadPress(key) {
        if (key === "C") keypadBuffer = "0.00";
        else if (key === "BS") keypadBuffer = keypadBuffer.length > 1 ? keypadBuffer.substring(0, keypadBuffer.length - 1) : "0.00";
        else if (key === "%") applyDiscount(discountRate > 0 ? 0.0 : 0.10);
        else keypadBuffer = (keypadBuffer === "0.00" || keypadBuffer === "0") ? key : keypadBuffer + key;
    }

    function addCustomItem() {
        let priceVal = parseFloat(keypadBuffer);
        if (!isNaN(priceVal) && priceVal > 0) {
            addItem({ id: "CUST_" + Math.floor(Math.random() * 1000), sku: "99" + Math.floor(Math.random() * 80 + 10), name: "Custom Item", price: priceVal });
            keypadBuffer = "0.00";
        }
    }

    function goToCheckout() {
        if (cartItems.length === 0) { statusNotification = qsTr("Cannot checkout with empty cart!"); return; }
        cashTenderedText = Math.ceil(getTotalAmount()).toString();
        paymentMethod = "CASH";
        rootWindow.state = "payment";
    }

    function processPayment() {
        lastPaidAmount = getTotalAmount();
        lastPaymentMethod = paymentMethod;
        lastTransactionId = "POS-2026-" + Math.floor(1000 + Math.random() * 9000);
        lastAuthCode = "XP-" + Math.floor(1000 + Math.random() * 9000);
        rootWindow.state = "receipt";
        statusNotification = qsTr("Payment of $%1 processed successfully!").arg(lastPaidAmount.toFixed(2));
    }

    function resetNewSale() {
        clearCart();
        discountRate = 0.0;
        keypadBuffer = "0.00";
        rootWindow.state = "catalog";
        statusNotification = qsTr("New sale started.");
    }

    // MANDATORY Keys.onPressed block per AGENTS.md rule
    Keys.onPressed: (event) => {
        if (rootWindow.state === "payment" || rootWindow.state === "receipt") {
            if (event.key === Qt.Key_Escape) { rootWindow.state = "cart"; event.accepted = true; return; }
            else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                if (rootWindow.state === "payment") processPayment();
                else if (rootWindow.state === "receipt") resetNewSale();
                event.accepted = true;
                return;
            }
        }
        if (event.key === Qt.Key_Q || event.key === Qt.Key_Escape) { Qt.quit(); event.accepted = true; return; }
        if (event.key === Qt.Key_F1) { clearCart(); event.accepted = true; }
        else if (event.key === Qt.Key_F2) { applyDiscount(discountRate > 0 ? 0.0 : 0.10); event.accepted = true; }
        else if (event.key === Qt.Key_F3) { rootWindow.state = "catalog"; event.accepted = true; }
        else if (event.key === Qt.Key_F4) { rootWindow.state = "cart"; event.accepted = true; }
        else if (event.key === Qt.Key_F12) { goToCheckout(); event.accepted = true; }
    }

    // --- Header Panel ---
    Panel {
        id: headerPanel
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 5
        color: "#0d1c2d"
        border.color: "#273647"
        title: qsTr("RapidPOS  |  %1").arg(storeName)
        titleColor: "#4AE183"

        Row {
            x: 1; y: 1; spacing: 2
            Text { text: qsTr("👤 %1  📶 ONLINE  🕒 09:41 AM").arg(cashierName); color: "#D4E4FA" }
            Button {
                label: rootWindow.state === "catalog" ? "[ 1 CATALOG ]" : " 1 CATALOG "
                normalColor: rootWindow.state === "catalog" ? "#06BB63" : "#122131"
                textColor: rootWindow.state === "catalog" ? "#051424" : "#D4E4FA"
                normalBorderStyle: rootWindow.state === "catalog" ? "double" : "single"
                onClicked: rootWindow.state = "catalog"
            }
            Button {
                label: rootWindow.state === "cart" ? qsTr("[ 2 CART (%1) ]").arg(getItemCount()) : qsTr(" 2 CART (%1) ").arg(getItemCount())
                normalColor: rootWindow.state === "cart" ? "#06BB63" : "#122131"
                textColor: rootWindow.state === "cart" ? "#051424" : "#D4E4FA"
                normalBorderStyle: rootWindow.state === "cart" ? "double" : "single"
                onClicked: rootWindow.state = "cart"
            }
            Button {
                label: rootWindow.state === "payment" ? "[ 3 PAYMENT ]" : " 3 PAYMENT "
                normalColor: rootWindow.state === "payment" ? "#06BB63" : "#122131"
                textColor: rootWindow.state === "payment" ? "#051424" : "#D4E4FA"
                normalBorderStyle: rootWindow.state === "payment" ? "double" : "single"
                onClicked: goToCheckout()
            }
            Button {
                visible: rootWindow.state === "receipt"
                label: "[ 4 RECEIPT ]"
                normalColor: "#06BB63"
                textColor: "#051424"
                normalBorderStyle: "double"
                onClicked: rootWindow.state = "receipt"
            }
        }
    }

    // --- Footer Panel ---
    Panel {
        id: footerPanel
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 3
        color: "#0d1c2d"
        border.color: "#273647"
        title: qsTr("STATUS & KEYBOARD SHORTCUTS")
        titleColor: "#4AE183"

        Row {
            x: 1; y: 1; spacing: 2
            Text {
                text: statusNotification.length > 0 ? statusNotification : qsTr("[F1] Clear  [F2] 10% Disc  [F3] Catalog  [F4] Cart  [F12] Pay  [Q/ESC] Quit")
                color: statusNotification.length > 0 ? "#FFB961" : "#A8E6CF"
            }
        }
    }

    // --- Main View Stack ---
    Item {
        id: mainWorkspace
        anchors.top: headerPanel.bottom
        anchors.bottom: footerPanel.top
        anchors.left: parent.left
        anchors.right: parent.right

        CatalogView {
            id: catalogView
            anchors.fill: parent
            products: rootWindow.products
            currentCategory: rootWindow.currentCategory
            cartItems: rootWindow.cartItems
            totalAmount: rootWindow.getTotalAmount()
            subtotal: rootWindow.getSubtotal()
            taxAmount: rootWindow.getTaxAmount()
            discountAmount: rootWindow.getDiscountAmount()
            discountRate: rootWindow.discountRate
            itemCount: rootWindow.getItemCount()

            onAddItemRequested: (prod) => rootWindow.addItem(prod)
            onCheckoutRequested: rootWindow.goToCheckout()
            onViewCartRequested: rootWindow.state = "cart"
            onCategorySelected: (cat) => rootWindow.currentCategory = cat
        }

        CartView {
            id: cartView
            anchors.fill: parent
            cartItems: rootWindow.cartItems
            totalAmount: rootWindow.getTotalAmount()
            subtotal: rootWindow.getSubtotal()
            taxAmount: rootWindow.getTaxAmount()
            discountRate: rootWindow.discountRate
            itemCount: rootWindow.getItemCount()
            keypadBuffer: rootWindow.keypadBuffer

            onUpdateQtyRequested: (sku, delta) => rootWindow.updateQty(sku, delta)
            onClearCartRequested: rootWindow.clearCart()
            onApplyDiscountRequested: (rate) => rootWindow.applyDiscount(rate)
            onCheckoutRequested: rootWindow.goToCheckout()
            onKeypadKeyPressed: (key) => rootWindow.handleKeypadPress(key)
            onCustomItemRequested: rootWindow.addCustomItem()
        }

        PaymentView {
            id: paymentView
            anchors.fill: parent
            totalAmount: rootWindow.getTotalAmount()
            itemCount: rootWindow.getItemCount()
            paymentMethod: rootWindow.paymentMethod
            cashTenderedText: rootWindow.cashTenderedText

            onPaymentMethodSelected: (method) => rootWindow.paymentMethod = method
            onCashTenderedChanged: (text) => rootWindow.cashTenderedText = text
            onConfirmPaymentRequested: rootWindow.processPayment()
            onBackToCartRequested: rootWindow.state = "cart"
        }

        SuccessView {
            id: successView
            anchors.fill: parent
            transactionId: rootWindow.lastTransactionId
            paidAmount: rootWindow.lastPaidAmount
            paymentMethod: rootWindow.lastPaymentMethod
            authCode: rootWindow.lastAuthCode

            onReceiptOptionClicked: (opt) => rootWindow.statusNotification = qsTr("Receipt option [%1] selected.").arg(opt)
            onNewSaleRequested: rootWindow.resetNewSale()
        }
    }
}
