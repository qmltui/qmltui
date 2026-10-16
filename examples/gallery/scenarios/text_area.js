/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("TextArea · Highlight Layer Editor");
    terminal.expectText("import QmlTui");
    terminal.expectText("lines:     10");
    terminal.expectText("canUndo:   false");
    terminal.expectText("readOnly:  false");
    terminal.expectText("spans: 7");
    terminal.marker("Initial highlighted editor");
    terminal.sleep(400);

    // Move to the end of the first line and insert a comment, triggering textRevision and canUndo.
    terminal.press("End");
    terminal.typeText(" // core");
    terminal.waitForText("canUndo:   true");
    terminal.expectText("// core");
    terminal.marker("Edited multi-line buffer");
    terminal.sleep(400);

    // Toggle readOnly and Syntax layer in the Inspector.
    terminal.clickText("[ ] readOnly");
    terminal.waitForText("readOnly:  true");
    terminal.expectText("READ-ONLY");
    terminal.sleep(300);

    terminal.clickText("Syntax layer");
    terminal.waitForText("spans: 0");
    terminal.sleep(300);

    terminal.clickText("Syntax layer");
    terminal.waitForText("spans: 7");
    terminal.sleep(300);
}
