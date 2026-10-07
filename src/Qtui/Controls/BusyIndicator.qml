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

    property int _frameIndex: 0

    readonly property list<string> _frames: {
        if (style === BusyIndicator.Style.Line) {
            return ["|", "/", "-", "\\"]
        } else if (style === BusyIndicator.Style.Pipe) {
            return ["┤", "┘", "┴", "└", "├", "┌", "┬", "┐"]
        } else if (style === BusyIndicator.Style.Block) {
            return ["▖", "▘", "▝", "▗"]
        } else if (style === BusyIndicator.Style.Bounce) {
            return ["[=   ]", "[ =  ]", "[  = ]", "[   =]", "[  = ]", "[ =  ]"]
        } else {
            // Default Dots braille spinner
            return ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
        }
    }

    readonly property string _currentSymbol: _frames[_frameIndex % _frames.length]
    text: _currentSymbol
    color: "default"
    bold: true

    NumberAnimation {
        target: control
        property: "_frameIndex"
        running: control.running && control.visible
        from: 0
        to: control._frames.length
        duration: control.interval * control._frames.length
        loops: -1
    }
}
