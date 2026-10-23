/** @param {Runner} app */
function main(app) {
    app.expectText("segmented");
    app.expectText("thresholds");
    app.sleep(800);
    app.sleep(800);
    app.sleep(800);
    app.captureText("progress_bar.txt");
    app.stopRecording();
}
