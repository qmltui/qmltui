import QtQml
import Qamui
import Qamui.Controls

Panel {
    id: root
    property string buffer: "0.00"
    property double totalAmount: 0.0
    property double subtotal: 0.0
    property double taxAmount: 0.0

    signal keyPressed(string key)
    signal customItemRequested()
    signal checkoutRequested()

    color: "#0d1c2d"
    border.color: "#273647"
    title: qsTr("🔢 KEYPAD & TOTAL DUE")
    titleColor: "#FFB961"

    Column {
        x: 1
        y: 1
        width: parent ? parent.width - 2 : 0
        spacing: 0

        Row {
            spacing: 2
            Text {
                text: qsTr("TOTAL DUE: $%1").arg(root.totalAmount.toFixed(2))
                color: "#4AE183"
                bold: true
            }
            Text {
                text: qsTr("ENTRY: $%1").arg(root.buffer)
                color: "#FFB961"
                bold: true
            }
        }

        Text { text: "─────────────────────────────────"; color: "#273647" }

        // Keypad Grid Rows
        Row {
            spacing: 1
            Button { text: " 7 "; onClicked: root.keyPressed("7"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " 8 "; onClicked: root.keyPressed("8"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " 9 "; onClicked: root.keyPressed("9"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " C "; textColor: "#FFFFFF"; onClicked: root.keyPressed("C"); background: Rectangle { color: "#800000"; border.color: "default"; border.style: "single" } }
        }
        Row {
            spacing: 1
            Button { text: " 4 "; onClicked: root.keyPressed("4"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " 5 "; onClicked: root.keyPressed("5"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " 6 "; onClicked: root.keyPressed("6"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " BS"; onClicked: root.keyPressed("BS"); background: Rectangle { color: "#808000"; border.color: "default"; border.style: "single" } }
        }
        Row {
            spacing: 1
            Button { text: " 1 "; onClicked: root.keyPressed("1"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " 2 "; onClicked: root.keyPressed("2"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " 3 "; onClicked: root.keyPressed("3"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " % "; textColor: "#FFB961"; onClicked: root.keyPressed("%"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
        }
        Row {
            spacing: 1
            Button { text: " 0 "; onClicked: root.keyPressed("0"); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: " . "; onClicked: root.keyPressed("."); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
            Button { text: "+ CUSTOM ITEM"; textColor: "#4AE183"; onClicked: root.customItemRequested(); background: Rectangle { color: "#1c2b3c"; border.color: "default"; border.style: "single" } }
        }

        Text { text: "─────────────────────────────────"; color: "#273647" }

        Button {
            background: Rectangle {
                color: "#06BB63"
                border.color: "default"
                border.style: "single"
            }
            text: "[F12] PROCEED TO PAYMENT"
            textColor: "#051424"
            onClicked: root.checkoutRequested()
        }
    }
}
