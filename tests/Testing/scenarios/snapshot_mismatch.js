function main(terminal) {
    terminal.waitForText("ASYNC READY");
    terminal.expectSnapshot("snapshots/mismatch.txt");
}
