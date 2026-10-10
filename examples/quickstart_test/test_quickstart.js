// The quickstart renders, the focused button reacts to Enter, and the status
// line updates. Kept deliberately small: this is what a newcomer's first run
// looks like.
/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Welcome to Qtui");
    terminal.expectText("[ Press Enter, or Q to quit ]");
    terminal.expectText("Status: ready");
    terminal.press("Enter");
    terminal.waitForText("Status: Clicked!");
    terminal.captureText("quickstart.txt");
}
