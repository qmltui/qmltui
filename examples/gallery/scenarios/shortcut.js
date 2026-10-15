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
    terminal.press("F5");
    terminal.expectText("F5 pressed 1");
    terminal.sleep(900);
    terminal.captureText("shortcut.txt");
    terminal.stopRecording();
}
