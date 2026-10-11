// The styled capture records colors and attributes of committed rows.
function main(terminal) {
    terminal.expectText("INLINE FIXTURE");
    terminal.press("F3");
    terminal.waitForScrollbackText("STYLED ROW");
    terminal.captureScrollbackStyled("styled.txt");
}
