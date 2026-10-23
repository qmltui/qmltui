/** @param {Runner} app */
function main(app) {
    app.expectText("contentY 0 of 40");
    app.sleep(700);
    app.press("PageDown");
    app.sleep(700);
    app.press("PageDown");
    app.sleep(800);
    app.captureText("scroll_view.txt");
    app.stopRecording();
}
