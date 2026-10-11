/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Employee Directory ---");
    terminal.sleep(1500);
    terminal.press("Down");
    terminal.sleep(600);
    terminal.press("Down");
    terminal.sleep(600);
    terminal.press("Down");
    terminal.sleep(600);
    terminal.press("Down");
    terminal.sleep(600);
    terminal.press("Down");
    terminal.sleep(1000);
    terminal.press("Up");
    terminal.sleep(600);
    terminal.press("Up");
    terminal.sleep(600);
    terminal.press("Up");
    terminal.sleep(1500);
    terminal.stopRecording();
}
