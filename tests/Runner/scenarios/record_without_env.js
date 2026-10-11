// Without QAMUI_RUNNER_RECORD the terminal bytes go nowhere, so recording fails.
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TESTRUNNER FIXTURE");
    terminal.startRecording("x.cast");
}
