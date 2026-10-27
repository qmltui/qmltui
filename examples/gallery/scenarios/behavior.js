/** @param {Runner} app */
function main(app) {
    app.expectText("Behavior on x");
    app.expectText("Target x: 0");
    app.sleep(700);
    app.press("Right");
    app.sleep(300);
    app.sleep(300);
    app.waitForText("Target x: 12");
    app.press("Right");
    app.sleep(300);
    app.sleep(300);
    app.waitForText("Target x: 24");
    app.press("Space");
    app.sleep(400);
    app.sleep(400);
    app.waitForText("colour: warm");
    app.captureText("behavior.txt");
    app.stopRecording();
}
