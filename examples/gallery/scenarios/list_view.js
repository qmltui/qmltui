/** @param {Runner} app */
function main(app) {
    app.expectText("Current: Ada Lovelace");
    app.sleep(800);
    app.press("Down");
    app.expectText("Current: Grace Hopper");
    app.sleep(700);
    app.press("Down");
    app.expectText("Current: Linus Torvalds");
    app.sleep(700);
    app.press("Down");
    app.expectText("Current: Margaret Hamilton");
    app.sleep(900);
    app.captureText("list_view.txt");
    app.stopRecording();
}
