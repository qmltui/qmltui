/** @param {Runner} app */
function main(app) {
    app.expectText("Easing.OutBounce");
    app.expectText("Easing.InOutQuad");
    app.sleep(500);
    app.sleep(500);
    app.sleep(500);
    app.sleep(500);
    app.captureText("animation.txt");
    app.stopRecording();
}
