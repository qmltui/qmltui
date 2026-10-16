/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("GridView · Module Matrix");
    terminal.expectText("currentIndex: 0");
    terminal.expectText("Module: ▣ TERM");
    terminal.expectText("Coord:  (0, 0)");
    terminal.expectText("cellSize: 12x3");
    terminal.marker("Initial grid state");
    terminal.sleep(400);

    // Move right across the first row.
    terminal.press("Right");
    terminal.waitForText("currentIndex: 1");
    terminal.expectText("Module: ¶ TEXT");
    terminal.expectText("Coord:  (1, 0)");
    terminal.sleep(300);

    terminal.press("Right");
    terminal.waitForText("currentIndex: 2");
    terminal.expectText("Module: ╭ RECT");
    terminal.sleep(300);

    // Move down to the next row.
    terminal.press("Down");
    terminal.waitForText("Module: ▦ GRID");
    terminal.expectText("2D cell GridView");
    terminal.marker("Navigated to GRID tile");
    terminal.sleep(400);

    // Jump to last tile with End.
    terminal.press("End");
    terminal.waitForText("currentIndex: 11");
    terminal.expectText("Module: ◈ CTRL");
    terminal.expectText("Buttons, Dialogs & Tabs");
    terminal.sleep(300);
}
