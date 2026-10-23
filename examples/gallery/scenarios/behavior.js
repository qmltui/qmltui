/** @param {Runner} app */
function main(app) {
    app.expectText("Behavior on x");
    app.expectText("Target x: 0");
    app.sleep(700);
    app.press("Right");
    app.sleep(400);
    app.expectText("Target x: 12");
    app.sleep(600);
    app.press("Right");
    app.sleep(400);
    app.expectText("Target x: 24");
    app.sleep(600);
    app.press("Space");
    app.sleep(500);
    app.expectText("colour: warm");
    app.sleep(800);
    app.captureText("behavior.txt");
    app.stopRecording();
}
