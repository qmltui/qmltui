/** @param {Runner} app */
function main(app) {
    app.expectText("Target: Production  ·  eu-west-1 (Dublin)  ·  log=info");
    app.sleep(600);
    app.press("ENTER");
    app.expectText("Staging");
    app.expectText("Development");
    app.sleep(600);
    app.press("DOWN");
    app.press("ENTER");
    app.expectText("Target: Staging  ·  eu-west-1 (Dublin)  ·  log=info");
    app.sleep(600);
    app.press("TAB");
    app.press("DOWN");
    app.expectText("Target: Staging  ·  us-east-1 (Virginia)  ·  log=info");
    app.sleep(800);
    app.captureText("combo_box.txt");
    app.stopRecording();
}
