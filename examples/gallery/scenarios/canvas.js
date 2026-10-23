/** @param {Runner} app */
function main(app) {
    app.expectText("Braille dot matrix");
    app.sleep(400);
    app.sleep(400);
    app.sleep(400);
    app.sleep(400);
    app.captureText("canvas.txt");
    app.stopRecording();
}
