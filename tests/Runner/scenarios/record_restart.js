// stopRecording() then startRecording() begins a second cast with a full
// repaint, so it stands alone.
/** @param {Runner} app */
function main(app) {
    app.expectText("TESTRUNNER FIXTURE");
    app.press("Tab");
    app.stopRecording();
    app.press("Tab");            // focus the fixture's TextInput
    app.typeText("one");
    app.expectText("INPUT: one");
    app.startRecording("second.cast");
    app.typeText("two");
    app.expectText("INPUT: onetwo");
    app.marker("second");
}
