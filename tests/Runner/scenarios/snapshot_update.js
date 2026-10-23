// Runs from a build-tree copy so QMLTUI_RUNNER_UPDATE_SNAPSHOTS=1 can rewrite the
// baseline next to it (the record test), and a following run without update
// mode verifies against the freshly recorded file (the verify test).
function main(app) {
    app.waitForText("TESTRUNNER FIXTURE");
    app.expectSnapshot("snapshots/updated_baseline.txt");
}
