// Scrollback rows are committed with the frame that carries them and are
// observed separately from the live region.
function main(app) {
    app.expectText("INLINE FIXTURE");
    if (app.scrollbackRowCount() !== 0)
        app.fail("expected an empty scrollback at start, got " + app.scrollbackRowCount());

    app.typeText("hello scroll");
    app.press("Enter");
    app.expectNoText("hello scroll");            // the prompt was cleared
    app.expectScrollbackText("hello scroll");     // and the line was committed

    app.press("F1");
    app.waitForScrollbackText("MODEL ROW 0");
    app.press("F2");
    app.waitForScrollbackText("wide 世界 row");
    if (app.scrollbackRowCount() !== 3)
        app.fail("expected three committed rows, got " + app.scrollbackRowCount());

    app.expectNoScrollbackText("INLINE FIXTURE"); // the live region is not scrollback
    app.captureScrollback("scrollback.txt");
}
