/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TESTRUNNER FIXTURE");
    try {
        terminal.fail("Caught failure must still terminate");
    } catch (error) {
        // A caught failure must not disable the JavaScript watchdog.
    }
    while (true) {}
}
