/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Behavior on x");
    terminal.expectText("Target x: 0");
    terminal.sleep(700);
    terminal.press("Right");
    terminal.sleep(400);
    terminal.expectText("Target x: 12");
    terminal.sleep(600);
    terminal.press("Right");
    terminal.sleep(400);
    terminal.expectText("Target x: 24");
    terminal.sleep(600);
    terminal.press("Space");
    terminal.sleep(500);
    terminal.expectText("colour: warm");
    terminal.sleep(800);
    terminal.captureText("behavior.txt");
    terminal.stopRecording();
}
