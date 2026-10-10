// A typo in an option key must fail the scenario, never pass silently.
function main(terminal) {
    terminal.waitForText("TESTRUNNER FIXTURE");
    terminal.waitForText("STATUS", { timout: 500 });
    terminal.fail("The unknown option must fail before this line");
}
