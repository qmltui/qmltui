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

    property int style: Spinner.Style.Dots
    property bool running: true
    property int interval: 100

    property int _frameIndex: 0

    readonly property list<string> _frames: {
        if (style === Spinner.Style.Line) {
            return ["|", "/", "-", "\\"]
        } else if (style === Spinner.Style.Pipe) {
            return ["┤", "┘", "┴", "└", "├", "┌", "┬", "┐"]
        } else if (style === Spinner.Style.Block) {
            return ["▖", "▘", "▝", "▗"]
        } else if (style === Spinner.Style.Bounce) {
            return ["[=   ]", "[ =  ]", "[  = ]", "[   =]", "[  = ]", "[ =  ]"]
        } else {
            // Default Dots braille spinner
            return ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"]
        }
    }

    readonly property string _currentSymbol: _frames[_frameIndex % _frames.length]
    text: _currentSymbol
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
