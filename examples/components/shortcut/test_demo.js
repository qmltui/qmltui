/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("theme: amber");
    terminal.sleep(1500);
    terminal.press("2");
    terminal.expectText("theme: mint");
    terminal.sleep(1000);
    terminal.press("3");
    terminal.expectText("theme: rose");
    terminal.sleep(1000);
    terminal.press("F5");
    terminal.expectText("F5 pressed 1");
    terminal.sleep(800);
    terminal.press("F5");
    terminal.expectText("F5 pressed 2");
    terminal.sleep(800);
    terminal.press("1");
    terminal.expectText("theme: amber");
    terminal.sleep(1500);
    terminal.stopRecording();
}
