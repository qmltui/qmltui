// Scrollback rows are committed with the frame that carries them and are
// observed separately from the live region.
function main(terminal) {
    terminal.expectText("INLINE FIXTURE");
    if (terminal.scrollbackRowCount() !== 0)
        terminal.fail("expected an empty scrollback at start, got " + terminal.scrollbackRowCount());

    terminal.typeText("hello scroll");
    terminal.press("Enter");
    terminal.expectNoText("hello scroll");            // the prompt was cleared
    terminal.expectScrollbackText("hello scroll");     // and the line was committed

    terminal.press("F1");
    terminal.waitForScrollbackText("MODEL ROW 0");
    terminal.press("F2");
    terminal.waitForScrollbackText("wide 世界 row");
    if (terminal.scrollbackRowCount() !== 3)
        terminal.fail("expected three committed rows, got " + terminal.scrollbackRowCount());

    terminal.expectNoScrollbackText("INLINE FIXTURE"); // the live region is not scrollback
    terminal.captureScrollback("scrollback.txt");
}
