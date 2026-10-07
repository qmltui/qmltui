function main(terminal) {
    terminal.waitForText("ASYNC READY");
    terminal.expectSnapshot("snapshots/fixture.txt");
}
