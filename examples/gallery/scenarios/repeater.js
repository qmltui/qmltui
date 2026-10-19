/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Count: 7");
    terminal.sleep(700);
    terminal.press("Right");
    terminal.press("Right");
    terminal.press("Right");
    terminal.expectText("Count: 10");
    terminal.sleep(800);
    terminal.press("Right");
    terminal.press("Right");
    terminal.expectText("Count: 12");
    terminal.sleep(900);
    terminal.captureText("repeater.txt");
    terminal.stopRecording();
}
