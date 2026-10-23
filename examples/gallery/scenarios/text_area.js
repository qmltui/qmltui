/** @param {Runner} app */
function main(app) {
    app.expectText("2 lines");
    app.sleep(700);
    app.press("End");
    app.press("Enter");
    app.typeText("Line 3 added live!");
    app.expectText("3 lines");
    app.sleep(900);
    app.captureText("text_area.txt");
    app.stopRecording();
}
