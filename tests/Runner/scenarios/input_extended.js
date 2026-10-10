// Hover gating, press-and-hold, drag motion, bracketed paste, and text keys.
function main(terminal) {
    terminal.waitForText("STATUS: READY");

    // Any-motion reporting is off, so the hover is dropped like a terminal
    // that never sends motion.
    terminal.hover(66, 16);
    terminal.expectNoText("MOUSE: ENTER");

    // Arm hover reporting and hover again: now the probe sees it.
    terminal.press("H");
    terminal.waitForText("HOVER ARMED");
    terminal.hover(66, 16);
    terminal.waitForText("MOUSE: ENTER");

    // Press-and-hold and release are separate events.
    terminal.mouseDown(66, 16);
    terminal.waitForText("MOUSE: DOWN");
    terminal.mouseUp(66, 16);
    terminal.waitForText("MOUSE: UP");

    // A drag reports intermediate held-motion cells, not one jump to the end.
    terminal.drag(61, 16, 70, 16);
    terminal.waitForText("MOUSE: UP 10,");
    if (!/DRAG TICKS: ([2-9]|[1-9][0-9])/.test(terminal.frameText()))
        terminal.fail("drag() did not report intermediate motion");

    // Bracketed paste: newline and tab arrive as data. The single-line input
    // maps them to spaces instead of acting on Return and Tab keys.
    terminal.clickText("type text here");
    terminal.paste("line1\nline2\ttab");
    terminal.waitForText("INPUT: line1 line2 tab");

    // press() accepts any single grapheme and routes it through the decoder.
    terminal.press("é");
    terminal.press("世");
    terminal.waitForText("INPUT: line1 line2 tabé世");
    terminal.captureText("input_extended.txt");
}
