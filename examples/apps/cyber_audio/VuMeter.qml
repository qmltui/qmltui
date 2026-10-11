pragma ComponentBehavior: Bound
import Qamui
import Qamui.Controls

// One VU channel: "L  -6.0 dB" caption over a threshold-colored bar. Two rows tall.
Column {
    id: meter
    property string label: "L"
    property real level: 0.0             // 0..1, full-scale sine = 1.0
    property string accentColor: Theme.vuLow
    property string labelColor: Theme.subtle     // matches the channel's scope trace
    property int meterWidth: 28

    width: meterWidth
    spacing: 0

    Row {
        spacing: 2
        Text {
            text: meter.label
            bold: true
            color: meter.labelColor
        }
        Text {
            text: Theme.formatDecibels(meter.level)
            color: meter.level > 0.85 ? Theme.hot : Theme.subtle
        }
    }
    ProgressBar {
        width: meter.meterWidth
        value: meter.level * 100
        from: 0
        to: 100
        thresholds: [
            { value: 65, color: meter.accentColor },
            { value: 85, color: Theme.warning },
            { value: 100, color: Theme.hot }
        ]
    }
}
