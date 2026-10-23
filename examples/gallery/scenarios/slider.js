/** @param {Runner} app */
function main(app) {
    app.expectText("Volume  42%");
    app.sleep(700);
    for (let i = 0; i < 8; ++i)
        app.press("Right");
    app.expectText("Volume  50%");
    app.sleep(700);
    app.press("Tab");
    app.sleep(500);
    app.press("Right");
    app.expectText("Contrast  4");
    app.sleep(900);
    app.captureText("slider.txt");
    app.stopRecording();
}
