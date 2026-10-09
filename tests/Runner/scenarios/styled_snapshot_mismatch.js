// The baseline keeps the fixture text but claims the STATUS row is white, so
// only the style section differs and the diff must call out the color.
function main(terminal) {
    terminal.waitForText("ASYNC READY");
    terminal.expectStyled("snapshots/mismatch_styled.txt");
}
