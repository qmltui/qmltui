/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("3 services, all healthy");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("gateway");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Enter");
    terminal.expectText("status Null");
    terminal.sleep(700);
    terminal.press("Enter");
    terminal.expectText("status Ready");
    terminal.sleep(900);
    terminal.captureText("loader.txt");
    terminal.stopRecording();
}
