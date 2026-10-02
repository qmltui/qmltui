import QtQml 2.15
import Terminal 1.0
import IndustrialPlcHmi 1.0

Column {
    id: root
    width: parent ? parent.width : 20
    spacing: 0
    property string label: ""
    property double value: 0
    property string unit: ""
    property double minVal: 0
    property double maxVal: 100
    property double warningVal: 75
    property double dangerVal: 85
    property int barWidth: Math.max(8, root.width - 2)
    property int decimals: 1

    Text {
        width: parent ? parent.width : 20
        text: root.label + ": " + (root.decimals > 0 ? root.value.toFixed(root.decimals) : root.value.toString()) + " " + root.unit
        color: root.value >= root.dangerVal ? HmiTheme.textDanger : (root.value >= root.warningVal ? HmiTheme.textWarning : HmiTheme.textPrimary)
        bold: root.value >= root.warningVal
    }

    Text {
        width: parent ? parent.width : 20
        text: HmiTheme.makeProgressBar(root.value, root.minVal, root.maxVal, root.barWidth)
        color: root.value >= root.dangerVal ? HmiTheme.statusError : (root.value >= root.warningVal ? HmiTheme.textWarning : HmiTheme.textHighlight)
    }
}
