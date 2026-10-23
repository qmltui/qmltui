/** @param {Runner} app */
function main(app) {
    app.expectText("No decision yet");
    app.sleep(800);
    app.press("Enter");
    app.expectText("report.pdf will be removed");
    app.sleep(900);
    app.press("Escape");
    app.expectText("Kept report.pdf");
    app.sleep(800);
    app.press("Enter");
    app.expectText("report.pdf will be removed");
    app.sleep(800);
    app.press("Enter");
    app.expectText("Deleted report.pdf");
    app.sleep(900);
    app.captureText("dialog.txt");
    app.stopRecording();
}
