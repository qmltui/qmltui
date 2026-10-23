// Without update mode, a missing baseline must fail and name the variable
// that records it.
function main(app) {
    app.waitForText("TESTRUNNER FIXTURE");
    app.expectSnapshot("snapshots/never_recorded.txt");
}
