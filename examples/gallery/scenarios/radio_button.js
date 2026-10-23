/** @param {Runner} app */
function main(app) {
    app.expectText("Profile: fast");
    app.sleep(800);
    app.press("Tab");
    app.sleep(500);
    app.press("Space");
    app.expectText("Profile: balanced");
    app.sleep(800);
    app.press("Tab");
    app.sleep(500);
    app.press("Space");
    app.expectText("Profile: safe");
    app.sleep(900);
    app.captureText("radio_button.txt");
    app.stopRecording();
}
