/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TABHEADER WIDGET DEMO");
    terminal.sleep(1500);
    terminal.press("Right");
    terminal.sleep(1000);
    terminal.press("Right");
    terminal.sleep(1000);
    terminal.press("Right");
    terminal.sleep(1000);
    terminal.press("Left");
    terminal.sleep(1000);
    terminal.press("Left");
    terminal.sleep(1500);
    terminal.stopRecording();
}
