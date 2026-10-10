// ToolTip.delay and ToolTip.timeout on the virtual clock, to the millisecond.
function main(terminal) {
    terminal.waitForText("Target");
    terminal.expectNoText("TIPTEXT");

    terminal.hover(4, 4);
    terminal.advanceTime(499);
    terminal.expectNoText("TIPTEXT");
    terminal.advanceTime(1);
    terminal.expectText("TIPTEXT");

    terminal.advanceTime(399);
    terminal.expectText("TIPTEXT");
    terminal.advanceTime(1);
    terminal.expectNoText("TIPTEXT");
}
