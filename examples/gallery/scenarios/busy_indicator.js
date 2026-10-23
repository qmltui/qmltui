/** @param {Runner} app */
function main(app) {
    app.expectText("Dots");
    app.expectText("Bounce");
    app.sleep(400);
    app.sleep(400);
    app.sleep(400);
    app.sleep(400);
    app.captureText("busy_indicator.txt");
    app.stopRecording();
}
