pragma ComponentBehavior: Bound
import QtQml
import QmlTui

// Single-row horizontal progress bar for `value` in `[from, to]`. Supports
// glyph presets (`style`), custom `fillCharacter`/`emptyCharacter`, optional
// `[` `]` brackets and percentage readout, optional `trackColor`, and opt-in
// `gradient` or step `thresholds`.
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

    // Style presets: "block" (█░, the default), "segmented" (▉░), "line" (━─),
    // "pill" (▮▯), "dot" (●○), "hash" (#-). fillCharacter/emptyCharacter override any preset.
    property string style: "block"
    // Defaults to terminal ink; gradients and thresholds are opt-in data colors.
    property color color
    // Optional explicit color for the unfilled portion; when unset, empty cells dim automatically.
    property color trackColor

    property string fillCharacter: ""
    property string emptyCharacter: ""

    property Gradient gradient: null
    // Step thresholds array, e.g. `[{ value: 60, color: "#34d399" }]`.
    property var thresholds: []

    readonly property bool hasGradient: (control.gradient && control.gradient.stops && control.gradient.stops.length > 0) || (control.thresholds && control.thresholds.length > 0)

    readonly property string effectiveFillChar: {
        if (fillCharacter && fillCharacter.length > 0)
            return fillCharacter;
        if (control.style === "segmented")
            return "▉";
        if (control.style === "line")
            return "━";
        if (control.style === "pill")
            return "▮";
        if (control.style === "dot")
            return "●";
        if (control.style === "hash")
            return "#";
        return "█"; // "block" and any unknown preset
    }

    readonly property string effectiveEmptyChar: {
        if (emptyCharacter && emptyCharacter.length > 0)
            return emptyCharacter;
        if (control.style === "segmented")
            return "░";
        if (control.style === "line")
            return "─";
        if (control.style === "pill")
            return "▯";
        if (control.style === "dot")
            return "○";
        if (control.style === "hash")
            return "-";
        return "░"; // "block" and any unknown preset
    }

    readonly property real position: {
        if (to <= from)
            return 0.0;
        return Math.max(0.0, Math.min(1.0, (value - from) / (to - from)));
    }

    readonly property var segments: {
        void d.gradientRevision;
        if (!control.hasGradient)
            return [];

        let w = control.barWidth;
        if (w <= 0)
            return [];

        let fillLen = Math.round(control.position * w);
        let emptyLen = w - fillLen;

        let fillChar = control.effectiveFillChar;
        let emptyChar = control.effectiveEmptyChar;

        let segs = [];
        let currentSegColor = null;
        let currentSegKey = "";
        let currentSegText = "";

        for (let i = 0; i < fillLen; ++i) {
            let posAtCell = w > 1 ? (i / (w - 1)) : 0.0;
            let c = getColorAt(posAtCell);
            let k = colorKey(c);
            if (k !== currentSegKey || currentSegText.length === 0) {
                if (currentSegText.length > 0) {
                    segs.push({
                        text: currentSegText,
                        color: currentSegColor
                    });
                }
                currentSegColor = c;
                currentSegKey = k;
                currentSegText = fillChar;
            } else {
                currentSegText += fillChar;
            }
        }
        if (currentSegText.length > 0) {
            segs.push({
                text: currentSegText,
                color: currentSegColor
            });
        }

        if (emptyLen > 0) {
            segs.push({
                text: emptyChar.repeat(emptyLen),
                color: (control.trackColor.valid && control.trackColor !== QmlTui.defaultColor) ? control.trackColor : control.color
            });
        }

        return segs;
    }

    function getColorAt(pos) {
        if (control.gradient && control.gradient.stops && control.gradient.stops.length > 0) {
            return control.gradient.colorAt(pos);
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

    function colorKey(c) {
        if (c === undefined || c === null)
            return "";
        if (typeof c === "string")
            return c;
        if (c.r !== undefined)
            return String(c.r) + "," + String(c.g) + "," + String(c.b) + "," + String(c.a) + "," + String(c.valid);
        return String(c);
    }

    function isValidColor(c) {
        if (!c)
            return false;
        if (typeof c === "string")
            return c.length > 0;
        return c.valid === true;
    }

    implicitHeight: 1
    implicitWidth: 15

    QtObject {
        id: d

        property int gradientRevision: 0
    }

    Connections {
        target: control.gradient
        function onUpdated() {
            d.gradientRevision++;
        }
    }

    Text {
        anchors.fill: parent
        color: control.color
        spans: {
            const out = [];
            const w = control.barWidth;

            if (control.showBrackets) {
                out.push(QmlTui.span("[", {
                    bold: true
                }));
            }

            if (w > 0) {
                if (control.hasGradient) {
                    const segs = control.segments;
                    const hasEmpty = Math.round(control.position * w) < w;
                    for (let i = 0; i < segs.length; ++i) {
                        const seg = segs[i];
                        const isEmptySeg = hasEmpty && (i === segs.length - 1);
                        if (isEmptySeg) {
                            if (control.trackColor.valid && control.trackColor !== QmlTui.defaultColor) {
                                out.push(QmlTui.span(seg.text, {
                                    color: control.trackColor,
                                    bold: false
                                }));
                            } else if (control.color.valid && control.color !== QmlTui.defaultColor) {
                                out.push(QmlTui.span(seg.text, {
                                    color: control.color,
                                    bold: false
                                }));
                            } else {
                                out.push(QmlTui.span(seg.text, {
                                    bold: false,
                                    dim: true
                                }));
                            }
                        } else if (control.isValidColor(seg.color) && seg.color !== QmlTui.defaultColor) {
                            out.push(QmlTui.span(seg.text, {
                                color: seg.color,
                                bold: true
                            }));
                        } else {
                            out.push(QmlTui.span(seg.text, {
                                bold: true
                            }));
                        }
                    }
                } else {
                    const fillLen = Math.round(control.position * w);
                    const emptyLen = w - fillLen;
                    if (fillLen > 0) {
                        out.push(QmlTui.span(control.effectiveFillChar.repeat(fillLen), {
                            bold: true
                        }));
                    }
                    if (emptyLen > 0) {
                        if (control.trackColor.valid && control.trackColor !== QmlTui.defaultColor) {
                            out.push(QmlTui.span(control.effectiveEmptyChar.repeat(emptyLen), {
                                color: control.trackColor,
                                bold: false
                            }));
                        } else {
                            out.push(QmlTui.span(control.effectiveEmptyChar.repeat(emptyLen), {
                                bold: false,
                                dim: true
                            }));
                        }
                    }
                }
            }

            if (control.showBrackets) {
                out.push(QmlTui.span("]", {
                    bold: true
                }));
            }

            if (control.showPercentage && control.percentageText.length > 0) {
                out.push(QmlTui.span(control.percentageText, {
                    bold: true
                }));
            }

            return out;
        }
    }
}
