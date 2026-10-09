function main(terminal) {
    terminal.waitForText("ASYNC READY");
    terminal.captureStyled("styled.txt");
    terminal.expectStyled("snapshots/fixture_styled.txt");
}
