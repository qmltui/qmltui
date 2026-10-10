function main(terminal) {
    terminal.click(5, 5);
    terminal.doubleClick(10, 5);
    terminal.drag(5, 5, 15, 5);
    terminal.scroll(5, 5, 0, 120);

    terminal.waitForRegex("STATUS:\\s+READY");
    terminal.expectRegex("TESTRUNNER\\s+FIXTURE");

    terminal.expectTextInRegion(0, 0, 80, 5, "TESTRUNNER FIXTURE");
    terminal.expectStyle(1, 1, { character: "T", foreground: "#FFFFFFFF" });

    if (!terminal.frameText().includes("TESTRUNNER FIXTURE"))
        terminal.fail("frameText() did not return the committed frame");
    const titleCell = terminal.cell(1, 1);
    if (titleCell.character !== "T" || titleCell.foreground !== "#FFFFFFFF")
        terminal.fail("cell() did not expose the rendered title cell");
    const matches = terminal.findText("type text here");
    if (matches.length !== 1 || matches[0].y !== 5)
        terminal.fail("findText() returned unexpected placeholder matches");

    const wideMatches = terminal.findText("界Z");
    if (wideMatches.length !== 1 || wideMatches[0].width !== 3)
        terminal.fail("findText() did not report the display width of wide text");
    const wideCell = terminal.cell(1, 9);
    const continuationCell = terminal.cell(2, 9);
    const trailingCell = terminal.cell(3, 9);
    if (wideCell.character !== "界" || wideCell.continuation
            || !continuationCell.continuation || trailingCell.character !== "Z")
        terminal.fail("cell() did not expose wide and continuation cells");

    const duplicates = terminal.findText("DUPLICATE");
    if (duplicates.length !== 2 || duplicates[0].y !== 11 || duplicates[1].y !== 12)
        terminal.fail("findText() did not return duplicate matches in grid order");
    terminal.clickText("DUPLICATE", 1);

    terminal.clickText("type text here");
    terminal.typeText("selector");
    terminal.waitForText("INPUT: selector");

    // cell() reports the full attribute set and uses "default" for the
    // terminal's own color, exactly like styledText().
    const attributeCell = terminal.cell(1, 1);
    for (const name of ["bold", "dim", "italic", "underline", "reversed",
                        "blink", "conceal", "strikethrough"]) {
        if (typeof attributeCell.attributes[name] !== "boolean")
            terminal.fail("cell() attribute " + name + " is not a boolean");
        if (attributeCell.attributes[name] !== false)
            terminal.fail("cell() attribute " + name + " expected false");
    }
    const plainCell = terminal.cell(30, 20);
    if (plainCell.foreground !== "default")
        terminal.fail("cell() foreground expected \"default\" but got " + plainCell.foreground);
    terminal.expectStyle(30, 20, { foreground: "default" });

    // The internal bridge callbacks must not leak onto the scenario facade.
    for (const name of ["beginOperation", "endOperation",
                        "onPromiseResolved", "onPromiseRejected"]) {
        if (terminal[name] !== undefined)
            terminal.fail("Internal method leaked onto the facade: " + name);
    }

    terminal.captureText("extended_features.txt");
}
