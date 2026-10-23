// Update mode cannot write into a Qt resource: the run must fail and leave
// the actual frame in the artifact directory instead.
function main(app) {
    app.waitForText("TESTRUNNER FIXTURE");
    app.expectSnapshot(":/qmltui-tests/resource_snapshot.js");
}
