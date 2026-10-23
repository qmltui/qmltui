/** @param {Runner} app */
function main(app) {
    app.expectText("cpu %");
    app.expectText("net MB/s");
    app.sleep(400);
    app.sleep(400);
    app.sleep(400);
    app.sleep(400);
    app.captureText("sparkline.txt");
    app.stopRecording();
}
