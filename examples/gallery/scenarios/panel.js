/** @param {Runner} app */
function main(app) {
    app.expectText("Default caption");
    app.expectText("Centred caption");
    app.expectText("Right caption");
    app.sleep(1200);
    app.captureText("panel.txt");
    app.stopRecording();
}
