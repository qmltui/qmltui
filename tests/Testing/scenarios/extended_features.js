function run() {
    Test.click(5, 5);
    Test.doubleClick(10, 5);
    Test.drag(5, 5, 15, 5);
    Test.scroll(5, 5, 0, 120);

    Test.waitForRegex("STATUS:\\s+READY");
    Test.expectRegex("TESTRUNNER\\s+FIXTURE");

    Test.expectTextInRegion(0, 0, 80, 5, "TESTRUNNER FIXTURE");
    Test.expectStyle(1, 1, { character: "T", foreground: "#FFFFFFFF" });

    if (!Test.frameText().includes("TESTRUNNER FIXTURE"))
        Test.fail("frameText() did not return the committed frame");
    const titleCell = Test.cell(1, 1);
    if (titleCell.character !== "T" || titleCell.foreground !== "#FFFFFFFF")
        Test.fail("cell() did not expose the rendered title cell");
    const matches = Test.findText("type text here");
    if (matches.length !== 1 || matches[0].y !== 5)
        Test.fail("findText() returned unexpected placeholder matches");

    const wideMatches = Test.findText("界Z");
    if (wideMatches.length !== 1 || wideMatches[0].width !== 3)
        Test.fail("findText() did not report the display width of wide text");
    const wideCell = Test.cell(1, 9);
    const continuationCell = Test.cell(2, 9);
    const trailingCell = Test.cell(3, 9);
    if (wideCell.character !== "界" || wideCell.continuation
            || !continuationCell.continuation || trailingCell.character !== "Z")
        Test.fail("cell() did not expose wide and continuation cells");

    const duplicates = Test.findText("DUPLICATE");
    if (duplicates.length !== 2 || duplicates[0].y !== 11 || duplicates[1].y !== 12)
        Test.fail("findText() did not return duplicate matches in grid order");
    Test.clickText("DUPLICATE", 1);

    Test.clickText("type text here");
    Test.typeText("selector");
    Test.waitForText("INPUT: selector");

    Test.captureText("extended_features.txt");
}
run();
