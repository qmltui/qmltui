pragma ComponentBehavior: Bound
import Qamui
import QtQml
import Qamui.Controls

// Lower panel: stereo VU deck on the left, Braille oscilloscope on the right. Either
// half can be hidden; the oscilloscope takes the full width when the VU deck is off.
Panel {
    id: panel
    required property AudioEngine engine
    required property string accentColor           // left trace and L caption
    required property string secondaryColor
    property string rightColor: Theme.scopeRight   // right trace and R caption
    property bool blendOverlaps: false             // Canvas.CellColorMode.Average on shared cells
    property bool showScope: true
    property bool showVu: true
    property int scopeArt: 0                       // 0 traces, 1 persistence, 2 ridge, 3 nebula

    readonly property string artTitle: {
        switch (scopeArt) {
        case 1: return "PHOSPHOR SCOPE"
        case 2: return "RIDGE PLOT"
        case 3: return "NEON NEBULA"
        default: return "BRAILLE OSCILLOSCOPE"
        }
    }

    color: Theme.panel
    title: showScope ? artTitle + (scopeArt === 3 ? "" : " · LP " + engine.scopeLowPassLabel) : "STEREO VU METERS"
    titleColor: Theme.subtle
    border.color: Theme.frame

    Row {
        id: scopeRow
        anchors.fill: parent
        anchors.topMargin: 2
        anchors.bottomMargin: 1
        anchors.leftMargin: 2
        anchors.rightMargin: 2
        spacing: 2

        // VU deck, vertically centred; drops its heading when space is tight.
        Item {
            id: vuDeck
            visible: panel.showVu
            width: panel.showVu ? 28 : 0
            height: parent.height

            readonly property bool compact: height < 7

            Column {
                width: parent.width
                height: vuDeck.compact ? 4 : 7
                y: Math.max(0, Math.floor((vuDeck.height - height) / 2))
                spacing: vuDeck.compact ? 0 : 1

                Text {
                    visible: !vuDeck.compact
                    text: "STEREO VU"
                    color: panel.secondaryColor
                    bold: true
                }
                VuMeter {
                    label: "L"
                    level: panel.engine.vuLeft
                    accentColor: Theme.vuLow
                    labelColor: panel.accentColor
                    meterWidth: vuDeck.width
                }
                VuMeter {
                    label: "R"
                    level: panel.engine.vuRight
                    accentColor: Theme.vuLow
                    labelColor: panel.rightColor
                    meterWidth: vuDeck.width
                }
            }
        }

        Rectangle {
            visible: panel.showVu && panel.showScope
            width: 1
            height: parent.height
            color: Theme.frame
        }

        // The traces are C++ CanvasShapes: QML binds the sample arrays and the canvas
        // repaints itself when they change. Right first so left wins shared cells.
        Canvas {
            id: scope
            visible: panel.showScope
            width: panel.showVu ? Math.max(4, scopeRow.width - vuDeck.width - 1 - scopeRow.spacing * 2)
                                : scopeRow.width
            height: parent.height
            renderMode: Canvas.RenderMode.Braille
            cellColorMode: panel.blendOverlaps ? Canvas.CellColorMode.Average : Canvas.CellColorMode.LastDot
            color: panel.accentColor

            NeonNebula {
                id: nebula
                visible: panel.scopeArt === 3
                color: Theme.accent
                sparkColor: Theme.scopeRight
            }
            Connections {
                target: panel.engine
                function onLevelsChanged() {
                    if (panel.visible && panel.showScope && panel.scopeArt === 3 && panel.engine.playing)
                        nebula.advance(panel.engine.bandLevels)
                }
            }

            // 0: plain stereo traces
            WaveformTrace {
                visible: panel.scopeArt === 0
                samples: panel.engine.waveformRight
                color: panel.rightColor
            }
            WaveformTrace {
                visible: panel.scopeArt === 0
                samples: panel.engine.waveformLeft
                color: panel.accentColor
            }
            // 1: phosphor persistence, both channels leave fading trails
            PersistenceTrace {
                visible: panel.scopeArt === 1
                samples: panel.engine.waveformRight
                color: panel.rightColor
                fadeColor: Theme.panel
            }
            PersistenceTrace {
                visible: panel.scopeArt === 1
                samples: panel.engine.waveformLeft
                color: panel.accentColor
                fadeColor: Theme.panel
            }
            // 2: ridge plot of the mono history
            RidgePlot {
                visible: panel.scopeArt === 2
                samples: panel.engine.waveformMono
                color: panel.accentColor
                fadeColor: Theme.frame
            }
        }
    }
}
