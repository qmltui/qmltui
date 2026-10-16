/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Text · Typography & Wrapping Lab");
    terminal.expectText("QmlTui Cell Typography & Horizontal Gradient");
    const heading = terminal.findText("QmlTui Cell Typography & Horizontal Gradient")[0];
    terminal.expectStyle(heading.x, heading.y, { character: "Q", foreground: "#2cde85" });
    terminal.expectStyle(heading.x + heading.width - 1, heading.y, { character: "t", foreground: "#c084fc" });
    terminal.expectText("Align: Left");
    terminal.expectText("Wrap:  WordWrap");
    terminal.expectText("implicitW:");
    terminal.expectText("implicitH:");
    terminal.marker("Initial WordWrap + Left alignment");
    terminal.sleep(400);

    // Cycle horizontal alignment with A: Left -> HCenter -> Right.
    terminal.typeText("a");
    terminal.waitForText("Align: HCenter");
    terminal.sleep(300);

    terminal.typeText("a");
    terminal.waitForText("Align: Right");
    terminal.marker("Right aligned text");
    terminal.sleep(400);

    // Cycle wrap mode with W: WordWrap -> WrapAnywhere -> NoWrap.
    terminal.typeText("w");
    terminal.waitForText("Wrap:  WrapAnywhere");
    terminal.sleep(300);

    terminal.typeText("w");
    terminal.waitForText("Wrap:  NoWrap");
    terminal.expectText("implicitH: 1");
    terminal.sleep(300);

    // Restore WordWrap and Left alignment.
    terminal.typeText("w");
    terminal.waitForText("Wrap:  WordWrap");
    terminal.typeText("a");
    terminal.waitForText("Align: Left");
    terminal.sleep(300);
}
