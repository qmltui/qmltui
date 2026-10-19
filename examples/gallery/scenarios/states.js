/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("state: (base)");
    terminal.sleep(700);
    terminal.press("Enter");
    terminal.sleep(500);
    terminal.expectText("state: on");
    terminal.expectText("ON");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.sleep(500);
    terminal.expectText("state: (base)");
    terminal.sleep(800);
    terminal.captureText("states.txt");
    terminal.stopRecording();
}
