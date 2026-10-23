// Without QMLTUI_RUNNER_RECORD the terminal bytes go nowhere, so recording fails.
/** @param {Runner} app */
function main(app) {
    app.expectText("TESTRUNNER FIXTURE");
    app.startRecording("x.cast");
}
