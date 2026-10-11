/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QT QUICK NATIVE TREEVIEW DEMO");
    terminal.sleep(1500);
    terminal.press("Down");
    terminal.sleep(600);
    terminal.press("Down");
    terminal.sleep(600);
    terminal.press("Enter");
    terminal.sleep(1000);
    terminal.press("Down");
    terminal.sleep(600);
    terminal.press("Down");
    terminal.sleep(600);
    terminal.press("Enter");
    terminal.sleep(1000);
    terminal.press("Enter");
    terminal.sleep(1500);
    terminal.stopRecording();
}
