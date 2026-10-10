function main(terminal) {
    terminal.advanceTime(100);
    terminal.fail("advanceTime() must fail on the real clock");
}
