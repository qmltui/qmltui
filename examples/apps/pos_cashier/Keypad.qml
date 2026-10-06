import QtQml
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
            Button { text: " 7 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("7") }
            Button { text: " 8 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("8") }
            Button { text: " 9 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("9") }
            Button { text: " C "; normalColor: "#800000"; textColor: "#FFFFFF"; normalBorderStyle: "single"; onClicked: root.keyPressed("C") }
        }
        Row {
            spacing: 1
            Button { text: " 4 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("4") }
            Button { text: " 5 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("5") }
            Button { text: " 6 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("6") }
            Button { text: " BS"; normalColor: "#808000"; normalBorderStyle: "single"; onClicked: root.keyPressed("BS") }
        }
        Row {
            spacing: 1
            Button { text: " 1 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("1") }
            Button { text: " 2 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("2") }
            Button { text: " 3 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("3") }
            Button { text: " % "; normalColor: "#1c2b3c"; textColor: "#FFB961"; normalBorderStyle: "single"; onClicked: root.keyPressed("%") }
        }
        Row {
            spacing: 1
            Button { text: " 0 "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed("0") }
            Button { text: " . "; normalColor: "#1c2b3c"; normalBorderStyle: "single"; onClicked: root.keyPressed(".") }
            Button { text: "+ CUSTOM ITEM"; normalColor: "#1c2b3c"; textColor: "#4AE183"; normalBorderStyle: "single"; onClicked: root.customItemRequested() }
        }

        Text { text: "─────────────────────────────────"; color: "#273647" }

        Button {
            text: "[F12] PROCEED TO PAYMENT"
            normalColor: "#06BB63"
            textColor: "#051424"
            normalBorderStyle: "single"
            onClicked: root.checkoutRequested()
        }
    }
}
