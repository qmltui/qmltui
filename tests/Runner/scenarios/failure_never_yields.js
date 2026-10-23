/** @param {Runner} app */
function main(app) {
    app.expectText("TESTRUNNER FIXTURE");
    try {
        app.fail("Caught failure must still terminate");
    } catch (error) {
        // A caught failure must not disable the JavaScript watchdog.
    }
    while (true) {}
}
