// A typo in an option key must fail the scenario, never pass silently.
function main(app) {
    app.waitForText("TESTRUNNER FIXTURE");
    app.waitForText("STATUS", { timout: 500 });
    app.fail("The unknown option must fail before this line");
}
