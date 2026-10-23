/** @param {Runner} app */
function main(app) {
    app.expectText("3 services healthy");
    app.sleep(800);
    app.press("Right");
    app.expectText("p99 latency 38 ms");
    app.sleep(800);
    app.press("Right");
    app.expectText("retrying upstream");
    app.sleep(900);
    app.captureText("tab_bar.txt");
    app.stopRecording();
}
