/** @param {Runner} app */
function main(app) {
    app.expectText("Opened 0 times");
    app.sleep(800);
    app.press("Enter");
    app.expectText("Export options");
    app.expectText("Opened 1 time");
    app.sleep(900);
    app.clickText("Compress output");
    app.sleep(700);
    app.captureText("popup_open.txt");
    app.clickText("│ Export │");
    app.expectNoText("Export options");
    app.sleep(900);
    app.captureText("popup.txt");
    app.stopRecording();
}
