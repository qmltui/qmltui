/** @param {Runner} app */
function main(app) {
    app.expectText("Toasts stack in a corner");
    app.sleep(700);
    app.press("Enter");
    app.expectText("Background sync started");
    app.sleep(700);
    app.clickText("Success");
    app.expectText("All changes written");
    app.sleep(700);
    app.clickText("Warning");
    app.expectText("Heap above 80%");
    app.sleep(900);
    app.captureText("toast.txt");
    app.stopRecording();
}
