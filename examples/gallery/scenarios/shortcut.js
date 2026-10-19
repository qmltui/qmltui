/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("theme: amber");
    terminal.sleep(700);
    terminal.press("2");
    terminal.expectText("theme: mint");
    terminal.sleep(700);
    terminal.press("3");
    terminal.expectText("theme: rose");
    terminal.sleep(700);
    terminal.press("r");
    terminal.expectText("R pressed 1");
    terminal.sleep(900);
    terminal.captureText("shortcut.txt");
    terminal.stopRecording();
}
