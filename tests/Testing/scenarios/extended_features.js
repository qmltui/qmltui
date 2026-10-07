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

    terminal.captureText("extended_features.txt");
}
