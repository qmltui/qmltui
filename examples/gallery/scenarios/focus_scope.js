/** @param {Runner} app */
function main(app) {
    app.expectText("Active scope: Servers");
    app.sleep(600);
    app.press("SPACE");
    app.expectText("Selected api-gateway-01");
    app.sleep(600);
    app.press("2");
    app.expectText("Active scope: Actions");
    app.press("SPACE");
    app.expectText("Triggered: Restart Service");
    app.sleep(600);
    app.press("1");
    app.expectText("Active scope: Servers");
    app.sleep(700);
    app.captureText("focus_scope.txt");
    app.stopRecording();
}
