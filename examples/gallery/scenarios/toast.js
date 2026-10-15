/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Toasts stack in a corner");
    terminal.sleep(700);
    terminal.press("Enter");
    terminal.expectText("Background sync started");
    terminal.sleep(700);
    terminal.clickText("Success");
    terminal.expectText("All changes written");
    terminal.sleep(700);
    terminal.clickText("Warning");
    terminal.expectText("Heap above 80%");
    terminal.sleep(900);
    terminal.captureText("toast.txt");
    terminal.stopRecording();
}
