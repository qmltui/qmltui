/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("TOASTMANAGER NOTIFICATIONS DEMO");
    terminal.sleep(1500);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.sleep(1200);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.sleep(1200);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.sleep(1500);
    terminal.stopRecording();
}
