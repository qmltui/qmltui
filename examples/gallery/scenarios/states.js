/** @param {Runner} app */
function main(app) {
    app.expectText("state: (base)");
    app.sleep(700);
    app.press("Enter");
    app.sleep(500);
    app.expectText("state: on");
    app.expectText("ON");
    app.sleep(800);
    app.press("Enter");
    app.sleep(500);
    app.expectText("state: (base)");
    app.sleep(800);
    app.captureText("states.txt");
    app.stopRecording();
}
