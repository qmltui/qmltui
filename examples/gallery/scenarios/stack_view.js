/** @param {Runner} app */
function main(app) {
    app.expectText("Page 1");
    app.sleep(800);
    app.press("Enter");
    app.expectText("Page 2");
    app.sleep(800);
    app.press("Enter");
    app.expectText("Page 3");
    app.sleep(800);
    app.clickText("Pop");
    app.expectText("Page 2");
    app.sleep(900);
    app.captureText("stack_view.txt");
    app.stopRecording();
}
