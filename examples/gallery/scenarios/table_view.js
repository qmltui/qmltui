/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Select a row or click a header");
    terminal.expectText("qmltui_gallery");
    terminal.sleep(700);
    terminal.press("Down");
    terminal.sleep(400);
    terminal.press("Down");
    terminal.sleep(500);
    terminal.press("Enter");
    terminal.expectText("Activated row 1: qmltui_gallery");
    terminal.sleep(800);
    terminal.clickText("CPU%");
    terminal.expectText("Header 2 clicked");
    terminal.sleep(900);
    terminal.captureText("table_view.txt");
    terminal.stopRecording();
}
