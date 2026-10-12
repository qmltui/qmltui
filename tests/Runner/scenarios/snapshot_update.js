// Runs from a build-tree copy so QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 can rewrite the
// baseline next to it (the record test), and a following run without update
// mode verifies against the freshly recorded file (the verify test).
function main(terminal) {
    terminal.waitForText("TESTRUNNER FIXTURE");
    terminal.expectSnapshot("snapshots/updated_baseline.txt");
}
