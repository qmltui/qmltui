// resize() commits a frame at the new dimensions: bound content reflows, the
// right-anchored marker moves, and size() reports the committed frame.
function main(terminal) {
    terminal.waitForText("SIZE: 80x24");

    const initial = terminal.size();
    if (initial.columns !== 80 || initial.rows !== 24)
        terminal.fail("size() expected 80x24, got " + initial.columns + "x" + initial.rows);
    if (terminal.findText("EDGE")[0].x !== 75)
        terminal.fail("EDGE marker is not anchored to the right at 80 columns");

    terminal.resize(40, 16);
    terminal.expectText("SIZE: 40x16");
    terminal.expectTextInRegion(35, 1, 5, 1, "EDGE");
    const shrunk = terminal.size();
    if (shrunk.columns !== 40 || shrunk.rows !== 16)
        terminal.fail("size() expected 40x16 after resize");

    terminal.resize(120, 40);
    terminal.expectText("SIZE: 120x40");
    if (terminal.findText("EDGE")[0].x !== 115)
        terminal.fail("EDGE marker did not follow the resize to 120 columns");
    terminal.captureText("resize.txt");
}
