/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QML STATES & TRANSITIONS DEMO");
    terminal.advanceTime(200);
    terminal.captureText("state_transitions.txt");
}
