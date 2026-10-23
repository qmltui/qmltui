function main(app) {
    app.click(5, 5);
    app.doubleClick(10, 5);
    app.drag(5, 5, 15, 5);
    app.scroll(5, 5, 0, 120);

    app.waitForRegex("STATUS:\\s+READY");
    app.expectRegex("TESTRUNNER\\s+FIXTURE");

    app.expectTextInRegion(0, 0, 80, 5, "TESTRUNNER FIXTURE");
    app.expectStyle(1, 1, { character: "T", foreground: "#FFFFFFFF" });

    if (!app.frameText().includes("TESTRUNNER FIXTURE"))
        app.fail("frameText() did not return the committed frame");
    const titleCell = app.cell(1, 1);
    if (titleCell.character !== "T" || titleCell.foreground !== "#FFFFFFFF")
        app.fail("cell() did not expose the rendered title cell");
    const matches = app.findText("type text here");
    if (matches.length !== 1 || matches[0].y !== 5)
        app.fail("findText() returned unexpected placeholder matches");

    const wideMatches = app.findText("界Z");
    if (wideMatches.length !== 1 || wideMatches[0].width !== 3)
        app.fail("findText() did not report the display width of wide text");
    const wideCell = app.cell(1, 9);
    const continuationCell = app.cell(2, 9);
    const trailingCell = app.cell(3, 9);
    if (wideCell.character !== "界" || wideCell.continuation
            || !continuationCell.continuation || trailingCell.character !== "Z")
        app.fail("cell() did not expose wide and continuation cells");

    const duplicates = app.findText("DUPLICATE");
    if (duplicates.length !== 2 || duplicates[0].y !== 11 || duplicates[1].y !== 12)
        app.fail("findText() did not return duplicate matches in grid order");
    app.clickText("DUPLICATE", 1);

    app.clickText("type text here");
    app.typeText("selector");
    app.waitForText("INPUT: selector");

    // cell() reports the full attribute set and uses "default" for the
    // terminal's own color, exactly like styledText().
    const attributeCell = app.cell(1, 1);
    for (const name of ["bold", "dim", "italic", "underline", "reversed",
                        "blink", "conceal", "strikethrough"]) {
        if (typeof attributeCell.attributes[name] !== "boolean")
            app.fail("cell() attribute " + name + " is not a boolean");
        if (attributeCell.attributes[name] !== false)
            app.fail("cell() attribute " + name + " expected false");
    }
    const plainCell = app.cell(30, 20);
    if (plainCell.foreground !== "default")
        app.fail("cell() foreground expected \"default\" but got " + plainCell.foreground);
    app.expectStyle(30, 20, { foreground: "default" });

    // The internal bridge callbacks must not leak onto the scenario facade.
    for (const name of ["beginOperation", "endOperation",
                        "onPromiseResolved", "onPromiseRejected"]) {
        if (app[name] !== undefined)
            app.fail("Internal method leaked onto the facade: " + name);
    }

    app.captureText("extended_features.txt");
}
