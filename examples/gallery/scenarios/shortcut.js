/** @param {Runner} app */
function main(app) {
    app.expectText("theme: amber");
    app.sleep(700);
    app.press("2");
    app.expectText("theme: mint");
    app.sleep(700);
    app.press("3");
    app.expectText("theme: rose");
    app.sleep(700);
    app.press("r");
    app.expectText("R pressed 1");
    app.sleep(900);
    app.captureText("shortcut.txt");
    app.stopRecording();
}
