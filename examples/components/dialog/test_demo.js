/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Dialog Example");
    terminal.sleep(1500);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("Confirm");
    terminal.sleep(1500);
    terminal.press("Escape");
    terminal.expectText("QmlTui Dialog Example");
    terminal.sleep(1000);
    terminal.press("Tab");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("Confirm");
    terminal.sleep(1000);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Enter");
    terminal.sleep(1500);
    terminal.stopRecording();
}
