import QtQml
import Qamui

Item {
    id: control

    property real value: 0.0
    property real from: 0.0
    property real to: 1.0

    // Decorations folded in from the former Gauge: "[" "]" around the bar and
    // a trailing percentage. Both take cells away from the bar itself.
    property bool showBrackets: false
    property bool showPercentage: false
    readonly property int barWidth: Math.max(0, width - (showBrackets ? 2 : 0) - (showPercentage ? 5 : 0))
    readonly property string percentageText: showPercentage ? (" " + String(Math.round(position * 100)).padStart(3, " ") + "%") : ""

    // Style presets: "block" (█░, the default), "segmented" (▰▱), "pill" (▮▯),
    // "dot" (●○), "hash" (#-). fillCharacter/emptyCharacter override any preset.
    property string style: "block"
    // Defaults to terminal ink; gradients and thresholds are opt-in data colors.
    property rgba color: "default"

    // Custom arbitrary character properties (override style presets when set)
    property string fillCharacter: ""
    property string emptyCharacter: ""

    // Opt-in Gradient type integration (uses Qamui Gradient & GradientStop)
    property Gradient gradient: null

    // Opt-in Step Thresholds array [{ value: 60, color: "#34d399" }]
    property var thresholds: []

    readonly property bool hasGradient: (control.gradient && control.gradient.stops && control.gradient.stops.length > 0) || (control.thresholds && control.thresholds.length > 0)

    // Effective filled character
    readonly property string effectiveFillChar: {
        if (fillCharacter && fillCharacter.length > 0) return fillCharacter;
        if (control.style === "segmented") return "▰";
        if (control.style === "pill") return "▮";
        if (control.style === "dot") return "●";
        if (control.style === "hash") return "#";
        return "█"; // "block" and any unknown preset
    }

    // Effective empty character
    readonly property string effectiveEmptyChar: {
        if (emptyCharacter && emptyCharacter.length > 0) return emptyCharacter;
        if (control.style === "segmented") return "▱";
        if (control.style === "pill") return "▯";
        if (control.style === "dot") return "○";
        if (control.style === "hash") return "-";
        return "░"; // "block" and any unknown preset
    }

    readonly property real position: {
        if (to <= from) return 0.0;
        return Math.max(0.0, Math.min(1.0, (value - from) / (to - from)));
    }

    // Function to calculate color at a normalized position (0.0 to 1.0) along the progress bar
    function getColorAt(pos) {
        if (control.gradient && control.gradient.stops && control.gradient.stops.length > 0) {
            let stops = control.gradient.stops;
            for (let i = 0; i < stops.length; ++i) {
                let stop = stops[i];
                if (pos <= stop.position) {
                    return stop.color;
                }
            }
            return stops[stops.length - 1].color;
        }

        if (control.thresholds && control.thresholds.length > 0) {
            for (let i = 0; i < control.thresholds.length; ++i) {
                let th = control.thresholds[i];
                let thPos = (control.to > control.from) ? (th.value - control.from) / (control.to - control.from) : 0.0;
                if (pos <= thPos) {
                    return th.color;
                }
            }
            return control.thresholds[control.thresholds.length - 1].color;
        }

        return control.color;
    }

    // Calculate colored text segments from left to right across the progress bar length
    readonly property var segments: {
        if (!control.hasGradient) return [];

        let w = control.barWidth;
        if (w <= 0) return [];

        let fillLen = Math.round(control.position * w);
        let emptyLen = w - fillLen;

        let fillChar = control.effectiveFillChar;
        let emptyChar = control.effectiveEmptyChar;

        let segs = [];
        let currentSegColor = "";
        let currentSegText = "";

        for (let i = 0; i < fillLen; ++i) {
            let posAtCell = w > 1 ? (i / (w - 1)) : 0.0;
            let c = getColorAt(posAtCell);
            if (c !== currentSegColor) {
                if (currentSegText.length > 0) {
                    segs.push({ text: currentSegText, color: currentSegColor });
                }
                currentSegColor = c;
                currentSegText = fillChar;
            } else {
                currentSegText += fillChar;
            }
        }
        if (currentSegText.length > 0) {
            segs.push({ text: currentSegText, color: currentSegColor });
        }

        // Add empty portion
        if (emptyLen > 0) {
            let emptyStr = "";
            for (let j = 0; j < emptyLen; ++j) emptyStr += emptyChar;
            segs.push({ text: emptyStr, color: control.color });
        }

        return segs;
    }

    implicitHeight: 1
    implicitWidth: 15

    // Fast Single-Text element for standard single color (default mode - 0 Repeater overhead)
    Text {
        visible: !control.hasGradient
        anchors.fill: parent
        color: control.color
        bold: true
        text: {
            if (control.hasGradient) return "";
            let w = control.barWidth;
            if (w <= 0) return control.showBrackets ? "[]" + control.percentageText : control.percentageText;

            let fillLen = Math.round(control.position * w);
            let emptyLen = w - fillLen;

            let fillChar = control.effectiveFillChar;
            let emptyChar = control.effectiveEmptyChar;

            let filledStr = "";
            for (let i = 0; i < fillLen; ++i) filledStr += fillChar;

            let emptyStr = "";
            for (let j = 0; j < emptyLen; ++j) emptyStr += emptyChar;

            const bar = filledStr + emptyStr;
            return (control.showBrackets ? "[" + bar + "]" : bar) + control.percentageText;
        }
    }

    // Segmented Row for Left-to-Right Gradient mode (only active when gradient is set)
    Row {
        visible: control.hasGradient
        anchors.fill: parent
        spacing: 0

        Text { visible: control.showBrackets; text: "["; color: control.color; bold: true }
        Repeater {
            model: control.hasGradient ? control.segments.length : 0

            Text {
                property var segData: control.segments[index]
                text: segData ? segData.text : ""
                color: segData ? segData.color : control.color
                bold: true
            }
        }
        Text { visible: control.showBrackets; text: "]"; color: control.color; bold: true }
        Text { visible: control.showPercentage; text: control.percentageText; color: control.color; bold: true }
    }
}
