import QtQml
import Qamui

// Phone-style Wi-Fi signal meter: four ascending bars plus a percent label.
// Falls back to full blocks on the framebuffer console, where the partial
// block glyphs are not available.
Row {
    id: meter

    property int strength: 0
    property bool showPercent: true

    readonly property int bars: strength <= 0 ? 0 : (strength < 30 ? 1 : (strength < 55 ? 2 : (strength < 80 ? 3 : 4)))
    readonly property string litRamp: Theme.simpleGlyphs ? "████" : "▂▄▆█"
    readonly property string dimRamp: Theme.simpleGlyphs ? "░░░░" : "▂▄▆█"
    readonly property string strengthColor: strength >= 70 ? Theme.statusOk : (strength >= 45 ? Theme.statusWarn : Theme.statusError)

    spacing: 1

    Row {
        Text {
            text: meter.litRamp.substring(0, meter.bars)
            color: meter.strengthColor
            bold: true
        }

        Text {
            text: meter.dimRamp.substring(meter.bars)
            color: Theme.textDim
        }
    }

    Text {
        visible: meter.showPercent
        text: (String(meter.strength) + "%").padStart(4)
        color: meter.strengthColor
    }
}
