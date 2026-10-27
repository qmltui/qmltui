/** @param {Runner} app */
function main(app) {
    app.expectText("14 lines");
    app.sleep(700);
    app.press("End");
    app.press("Enter");
    app.typeText("Line added live!");
    app.expectText("15 lines");
    app.sleep(900);
    app.captureText("text_area.txt");
    app.stopRecording();
}
