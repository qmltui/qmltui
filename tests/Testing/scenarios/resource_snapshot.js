function main(terminal) {
    terminal.waitForText("STATUS: ASYNC READY");
    terminal.expectSnapshot("snapshots/fixture.txt");
}
