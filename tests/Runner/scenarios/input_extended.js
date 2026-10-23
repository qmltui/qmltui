// Hover gating, press-and-hold, drag motion, bracketed paste, and text keys.
function main(app) {
    app.waitForText("STATUS: READY");

    // Any-motion reporting is off, so the hover is dropped like a terminal
    // that never sends motion.
    app.hover(66, 16);
    app.expectNoText("MOUSE: ENTER");

    // Arm hover reporting and hover again: now the probe sees it.
    app.press("H");
    app.waitForText("HOVER ARMED");
    app.hover(66, 16);
    app.waitForText("MOUSE: ENTER");

    // Press-and-hold and release are separate events.
    app.mouseDown(66, 16);
    app.waitForText("MOUSE: DOWN");
    app.mouseUp(66, 16);
    app.waitForText("MOUSE: UP");

    // A drag reports intermediate held-motion cells, not one jump to the end.
    app.drag(61, 16, 70, 16);
    app.waitForText("MOUSE: UP 10,");
    if (!/DRAG TICKS: ([2-9]|[1-9][0-9])/.test(app.frameText()))
        app.fail("drag() did not report intermediate motion");

    // Bracketed paste: newline and tab arrive as data. The single-line input
    // maps them to spaces instead of acting on Return and Tab keys.
    app.clickText("type text here");
    app.paste("line1\nline2\ttab");
    app.waitForText("INPUT: line1 line2 tab");

    // press() accepts any single grapheme and routes it through the decoder.
    app.press("é");
    app.press("世");
    app.waitForText("INPUT: line1 line2 tabé世");
    app.captureText("input_extended.txt");
}
