/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("RichText · Spans, HTML & ANSI");
    terminal.expectText("1. Text.spans + QmlTui.span()");
    terminal.expectText("2. Text.RichText (HTML + Hyperlink)");
    terminal.expectText("3. Text.AnsiText (VT SGR Escape Codes)");
    terminal.expectText("Theme:    Emerald");
    terminal.expectText("BoldBase: false");
    terminal.marker("Initial rich text comparison");
    terminal.sleep(400);

    // Cycle theme preset with T and toggle inherited span boldness with B.
    terminal.typeText("t");
    terminal.waitForText("Theme:    Cyan");
    terminal.sleep(300);

    terminal.typeText("b");
    terminal.waitForText("BoldBase: true");
    terminal.sleep(300);

    // Activate the embedded hyperlink in the RichText specimen.
    terminal.clickText("[docs://text]");
    terminal.waitForText("Link:     docs://text");
    terminal.marker("Activated hyperlink");
    terminal.sleep(400);

    // Toggle WordWrap with W.
    terminal.typeText("w");
    terminal.waitForText("RichWrap: NoWrap");
    terminal.sleep(300);

    terminal.typeText("w");
    terminal.waitForText("RichWrap: WordWrap");
    terminal.sleep(300);
}
