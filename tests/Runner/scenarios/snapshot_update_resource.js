// Update mode cannot write into a Qt resource: the run must fail and leave
// the actual frame in the artifact directory instead.
function main(terminal) {
    terminal.waitForText("TESTRUNNER FIXTURE");
    terminal.expectSnapshot(":/qamui-tests/resource_snapshot.js");
}
