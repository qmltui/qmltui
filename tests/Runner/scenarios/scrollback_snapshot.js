// A golden baseline of committed rows: a printed line, a wide-character row
// and a styled row, compared as normalized text.
function main(app) {
    app.expectText("INLINE FIXTURE");
    app.typeText("first line");
    app.press("Enter");
    app.press("F2");
    app.press("F3");
    app.waitForScrollbackText("STYLED ROW");
    app.expectScrollbackSnapshot("snapshots/scrollback.txt");
}
