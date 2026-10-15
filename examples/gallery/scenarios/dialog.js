/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("No decision yet");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("report.pdf will be removed");
    terminal.sleep(900);
    terminal.press("Escape");
    terminal.expectText("Kept report.pdf");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("report.pdf will be removed");
    terminal.sleep(800);
    terminal.press("Enter");
    terminal.expectText("Deleted report.pdf");
    terminal.sleep(900);
    terminal.captureText("dialog.txt");
    terminal.stopRecording();
}
