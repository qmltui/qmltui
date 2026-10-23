/** @param {Runner} app */
function main(app) {
    app.expectText("READY");
    app.expectText("gradient text flows");
    app.sleep(1200);
    app.captureText("text.txt");
    app.stopRecording();
}
