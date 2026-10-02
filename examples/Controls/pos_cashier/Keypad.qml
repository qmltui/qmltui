import QtQuick
import Qtui
import Qtui.Controls

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
        width: parent.width - 2
        spacing: 1

        Text {
            text: qsTr("TOTAL DUE: $%1").arg(root.totalAmount.toFixed(2))
            color: "#4AE183"
            bold: true
        }
        Text {
            text: qsTr("SUBTOTAL: $%1 | TAX: $%2").arg(root.subtotal.toFixed(2)).arg(root.taxAmount.toFixed(2))
            color: "#D4E4FA"
        }

        Text { text: "─────────────────────────────────"; color: "#273647" }

        Text {
            text: qsTr("KEYPAD ENTRY: $%1").arg(root.buffer)
            color: "#FFB961"
            bold: true
        }

        // Keypad Grid Rows
        Row {
            spacing: 1
            Button { label: " 7 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("7") }
            Button { label: " 8 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("8") }
            Button { label: " 9 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("9") }
            Button { label: " C "; normalColor: "#800000"; textColor: "#FFFFFF"; normalBorderStyle: "single"; onClicked: root.keyPressed("C") }
        }
        Row {
            spacing: 1
            Button { label: " 4 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("4") }
            Button { label: " 5 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("5") }
            Button { label: " 6 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("6") }
            Button { label: "BS "; normalColor: "#808000"; normalBorderStyle: "single"; onClicked: root.keyPressed("BS") }
        }
        Row {
            spacing: 1
            Button { label: " 1 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("1") }
            Button { label: " 2 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("2") }
            Button { label: " 3 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("3") }
            Button { label: " % "; normalColor: "#1c2b3c"; textColor: "#FFB961"; normalBorderStyle: "single"; onClicked: root.keyPressed("%") }
        }
        Row {
            spacing: 1
            Button { label: " 0 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("0") }
            Button { label: " . "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed(".") }
            Button { label: "+ CUSTOM ITEM"; normalColor: "#1c2b3c"; textColor: "#4AE183"; normalBorderStyle: "single"; onClicked: root.customItemRequested() }
        }

        Text { text: "─────────────────────────────────"; color: "#273647" }

        Button {
            label: "💳 PROCEED TO PAYMENT [F12]"
            normalColor: "#06BB63"
            textColor: "#051424"
            normalBorderStyle: "single"
            onClicked: root.checkoutRequested()
        }
    }
}
