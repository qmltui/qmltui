// A RECORD scenario: the cast opens with the first frame, carries later output
// and the markers placed between steps.
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TESTRUNNER FIXTURE");
    terminal.press("Tab");
    terminal.marker("start");
    terminal.press("Tab");            // focus the fixture's TextInput
    terminal.typeText("cast");
    terminal.expectText("INPUT: cast");
    terminal.sleep(50);
    terminal.marker("typed");
    terminal.resize(90, 26);
    terminal.expectText("SIZE: 90x26");
}
