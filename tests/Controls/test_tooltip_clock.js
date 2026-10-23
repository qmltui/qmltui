// ToolTip.delay and ToolTip.timeout on the virtual clock, to the millisecond.
function main(app) {
    app.waitForText("Target");
    app.expectNoText("TIPTEXT");

    app.hover(4, 4);
    app.advanceTime(499);
    app.expectNoText("TIPTEXT");
    app.advanceTime(1);
    app.expectText("TIPTEXT");

    app.advanceTime(399);
    app.expectText("TIPTEXT");
    app.advanceTime(1);
    app.expectNoText("TIPTEXT");
}
