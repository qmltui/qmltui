/** @param {Runner} app */
function main(app) {
    app.expectText("Count: 7");
    app.sleep(700);
    app.press("Right");
    app.press("Right");
    app.press("Right");
    app.expectText("Count: 10");
    app.sleep(800);
    app.press("Right");
    app.press("Right");
    app.expectText("Count: 12");
    app.sleep(900);
    app.captureText("repeater.txt");
    app.stopRecording();
}
