/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Interactive TextArea Component Demo");
    terminal.sleep(1500);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.typeText("Hello from QmlTui!");
    terminal.sleep(1000);
    terminal.press("Enter");
    terminal.typeText("A retained-mode terminal UI.");
    terminal.sleep(1500);
    terminal.stopRecording();
}
