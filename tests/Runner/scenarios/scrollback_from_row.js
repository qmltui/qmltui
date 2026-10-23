// scrollbackRowCount() is a mark: the fromRow parameter, positional or as an
// option, limits the other methods to rows committed after it.
function main(app) {
    app.expectText("INLINE FIXTURE");
    app.press("F1");
    app.waitForScrollbackText("MODEL ROW 0");
    const mark = app.scrollbackRowCount();
    if (mark !== 1)
        app.fail("expected one row before the mark, got " + mark);

    app.press("F1");
    app.waitForScrollbackText("MODEL ROW 1");
    const since = app.scrollbackText(mark);
    if (since.indexOf("MODEL ROW 0") >= 0)
        app.fail("a row before the mark leaked into scrollbackText(mark): " + since);
    if (since.indexOf("MODEL ROW 1") < 0)
        app.fail("the row after the mark is missing: " + since);
    if (app.scrollbackText({ fromRow: mark }) !== since)
        app.fail("the options form differs from the positional form");
    if (app.scrollbackText().indexOf("MODEL ROW 0") < 0)
        app.fail("the full text lost the first row");
    app.captureScrollback("since.txt", mark);
}
