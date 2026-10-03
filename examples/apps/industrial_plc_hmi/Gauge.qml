import QtQml 2.15
import Qtui
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

    property double animatedValue: root.value

    Behavior on animatedValue {
        NumberAnimation {
            duration: 350
            easing: Easing.OutQuad
        }
    }

    Text {
        width: parent ? parent.width : 20
        text: root.label + ": " + (root.decimals > 0 ? root.animatedValue.toFixed(root.decimals) : Math.round(root.animatedValue).toString()) + " " + root.unit
        color: root.animatedValue >= root.dangerVal ? HmiTheme.textDanger : (root.animatedValue >= root.warningVal ? HmiTheme.textWarning : HmiTheme.textPrimary)
        bold: root.animatedValue >= root.warningVal
    }

    Text {
        width: parent ? parent.width : 20
        text: HmiTheme.makeProgressBar(root.animatedValue, root.minVal, root.maxVal, root.barWidth)
        color: root.animatedValue >= root.dangerVal ? HmiTheme.statusError : (root.animatedValue >= root.warningVal ? HmiTheme.textWarning : HmiTheme.textHighlight)
    }
}
