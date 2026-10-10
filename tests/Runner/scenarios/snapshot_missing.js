// Without update mode, a missing baseline must fail and name the variable
// that records it.
function main(terminal) {
    terminal.waitForText("TESTRUNNER FIXTURE");
    terminal.expectSnapshot("snapshots/never_recorded.txt");
}
