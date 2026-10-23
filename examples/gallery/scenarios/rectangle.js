/** @param {Runner} app */
function main(app) {
    app.expectText("single");
    app.expectText("double");
    app.expectText("rounded");
    app.expectText("horizontal gradient");
    app.sleep(1200);
    app.captureText("rectangle.txt");
    app.stopRecording();
}
