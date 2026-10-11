// A golden baseline of committed rows: a printed line, a wide-character row
// and a styled row, compared as normalized text.
function main(terminal) {
    terminal.expectText("INLINE FIXTURE");
    terminal.typeText("first line");
    terminal.press("Enter");
    terminal.press("F2");
    terminal.press("F3");
    terminal.waitForScrollbackText("STYLED ROW");
    terminal.expectScrollbackSnapshot("snapshots/scrollback.txt");
}
