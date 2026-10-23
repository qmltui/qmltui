/** @param {Runner} app */
function main(app) {
    app.expectText("3 services, all healthy");
    app.sleep(800);
    app.press("Enter");
    app.expectText("gateway");
    app.sleep(800);
    app.press("Tab");
    app.sleep(500);
    app.press("Enter");
    app.expectText("status Null");
    app.sleep(700);
    app.press("Enter");
    app.expectText("status Ready");
    app.sleep(900);
    app.captureText("loader.txt");
    app.stopRecording();
}
