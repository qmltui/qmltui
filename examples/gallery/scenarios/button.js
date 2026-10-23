/** @param {Runner} app */
function main(app) {
    app.expectText("No clicks yet");
    app.sleep(800);
    app.press("Space");
    app.expectText("Clicked 1 time");
    app.sleep(700);
    app.press("Tab");
    app.sleep(500);
    app.press("Enter");
    app.expectText("Clicked 2 times");
    app.sleep(700);
    app.clickText("Pin");
    app.expectText("Pinned");
    app.sleep(900);
    app.captureText("button.txt");
    app.stopRecording();
}
