import QtQml
import Qtui

Text {
    id: control

    enum Style {
        Dots,
        Line,
        Pipe,
        Block,
        Bounce
    }

    property int style: BusyIndicator.Style.Dots
    property bool running: true
    property int interval: 100

    QtObject {
        id: d
        property int frameIndex: 0
        readonly property list<string> frames: {
            if (control.style === BusyIndicator.Style.Line) {
                return ["|", "/", "-", "\\"]
            } else if (control.style === BusyIndicator.Style.Pipe) {
                return ["┤", "┘", "┴", "└", "├", "┌", "┬", "┐"]
            } else if (control.style === BusyIndicator.Style.Block) {
                return ["▖", "▘", "▝", "▗"]
            } else if (control.style === BusyIndicator.Style.Bounce) {
                return ["[=   ]", "[ =  ]", "[  = ]", "[   =]", "[  = ]", "[ =  ]"]
            } else {
                // Default Dots braille spinner
                return ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
            }
        }
    }

    text: d.frames[d.frameIndex % d.frames.length]
    color: "default"
    bold: true

    NumberAnimation {
        target: d
        property: "frameIndex"
        running: control.running && control.visible
        from: 0
        to: d.frames.length
        duration: control.interval * d.frames.length
        loops: -1
    }
}
