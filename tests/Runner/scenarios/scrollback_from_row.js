// scrollbackRowCount() is a mark: the fromRow parameter, positional or as an
// option, limits the other methods to rows committed after it.
function main(terminal) {
    terminal.expectText("INLINE FIXTURE");
    terminal.press("F1");
    terminal.waitForScrollbackText("MODEL ROW 0");
    const mark = terminal.scrollbackRowCount();
    if (mark !== 1)
        terminal.fail("expected one row before the mark, got " + mark);

    terminal.press("F1");
    terminal.waitForScrollbackText("MODEL ROW 1");
    const since = terminal.scrollbackText(mark);
    if (since.indexOf("MODEL ROW 0") >= 0)
        terminal.fail("a row before the mark leaked into scrollbackText(mark): " + since);
    if (since.indexOf("MODEL ROW 1") < 0)
        terminal.fail("the row after the mark is missing: " + since);
    if (terminal.scrollbackText({ fromRow: mark }) !== since)
        terminal.fail("the options form differs from the positional form");
    if (terminal.scrollbackText().indexOf("MODEL ROW 0") < 0)
        terminal.fail("the full text lost the first row");
    terminal.captureScrollback("since.txt", mark);
}
