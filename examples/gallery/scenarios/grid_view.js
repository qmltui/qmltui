/** @param {Runner} app */
function main(app) {
    app.expectText("apple");
    app.expectText("mango");
    app.sleep(800);
    app.press("Down");
    app.sleep(700);
    app.press("Down");
    app.sleep(800);
    app.captureText("grid_view.txt");
    app.stopRecording();
}
