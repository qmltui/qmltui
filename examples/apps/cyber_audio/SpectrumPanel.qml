pragma ComponentBehavior: Bound
import Qtui
import Qtui.Controls

// 24-band equalizer. Each row is a single Text covering every band, rendered at
// 1/8-cell resolution with peak-hold caps; bar width adapts to the panel width.
Panel {
    id: panel
    required property AudioEngine engine
    required property string accentColor
    required property string secondaryColor

    readonly property int bandCount: engine.bandCount

    color: Theme.panel
    title: "24-BAND NEON EQUALIZER"
    titleColor: Theme.equalizerTitle
    border.color: Theme.frame

    // Builds the string for one row of the equalizer. `row` is 0 at the bottom.
    function spectrumRow(row, rows, barWidth, gap, pad) {
        const levels = panel.engine.bandLevels
        const peaks = panel.engine.peakLevels
        let line = " ".repeat(pad)
        const sep = " ".repeat(gap)
        for (let i = 0; i < panel.bandCount; ++i) {
            const fill = levels[i] * rows - row
            let glyph
            if (fill >= 1.0) {
                glyph = "█"
            } else if (fill > 0.0) {
                glyph = Theme.blockGlyphs[Math.round(fill * 8)]
            } else {
                const peakRow = Math.min(rows - 1, Math.floor(peaks[i] * rows))
                glyph = (peakRow === row && peaks[i] > 0) ? "▔" : " "
            }
            line += glyph.repeat(barWidth)
            if (i < panel.bandCount - 1)
                line += sep
        }
        return line
    }

    // Frequency labels aligned under each bar. Narrow pitches show every other label.
    function labelRow(barWidth, gap, pad, maxWidth) {
        const labels = panel.engine.bandLabels
        const pitch = barWidth + gap
        const showAll = pitch >= 4
        let line = " ".repeat(pad)
        for (let i = 0; i < panel.bandCount; ++i) {
            const label = (showAll || i % 2 === 0) ? labels[i] : ""
            line += label.padEnd(pitch, " ")
        }
        return line.substring(0, maxWidth)
    }

    Item {
        id: eqArea
        anchors.fill: parent
        anchors.topMargin: 2       // border row + one blank row below the title
        anchors.bottomMargin: 1
        anchors.leftMargin: 2
        anchors.rightMargin: 2

        readonly property int barRows: Math.max(1, height - 2)   // minus gap + label row
        readonly property int gap: width >= panel.bandCount * 5 ? 2 : 1
        readonly property int barWidth: Math.max(1,
            Math.floor((width - (panel.bandCount - 1) * gap) / panel.bandCount))
        readonly property int usedWidth: panel.bandCount * barWidth + (panel.bandCount - 1) * gap
        readonly property int leftPad: Math.max(0, Math.floor((width - usedWidth) / 2))

        // Bars grow upward from the row just above the label line.
        Column {
            anchors.top: parent.top
            anchors.left: parent.left
            width: parent.width
            height: eqArea.barRows
            spacing: 0

            Repeater {
                model: eqArea.barRows
                delegate: Text {
                    required property int index
                    readonly property int rowFromBottom: eqArea.barRows - 1 - index
                    readonly property real heightRatio: (rowFromBottom + 1) / eqArea.barRows
                    height: 1
                    bold: true
                    text: panel.spectrumRow(rowFromBottom, eqArea.barRows,
                                            eqArea.barWidth, eqArea.gap, eqArea.leftPad)
                    color: heightRatio > 0.8 ? panel.secondaryColor
                         : heightRatio > 0.5 ? Theme.mid : panel.accentColor
                }
            }
        }

        Text {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            height: 1
            text: panel.labelRow(eqArea.barWidth, eqArea.gap, eqArea.leftPad, eqArea.width)
            color: Theme.dim
        }
    }
}
