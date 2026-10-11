// stopRecording() then startRecording() begins a second cast with a full
// repaint, so it stands alone.
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TESTRUNNER FIXTURE");
    terminal.press("Tab");
    terminal.stopRecording();
    terminal.press("Tab");            // focus the fixture's TextInput
    terminal.typeText("one");
    terminal.expectText("INPUT: one");
    terminal.startRecording("second.cast");
    terminal.typeText("two");
    terminal.expectText("INPUT: onetwo");
    terminal.marker("second");
}
