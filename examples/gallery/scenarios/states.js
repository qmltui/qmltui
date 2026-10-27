/** @param {Runner} app */
function main(app) {
    app.expectText("state: (base)");
    app.sleep(700);
    app.press("Enter");
    app.sleep(500);
    app.expectText("state: on");
    app.expectText("ONLINE (ON)");
    app.sleep(800);
    app.clickText("Overload");
    app.sleep(500);
    app.expectText("state: overload");
    app.expectText("OVERLOAD");
    app.captureText("states_overload.txt");
    app.sleep(800);
    app.clickText("Reset");
    app.sleep(500);
    app.expectText("state: (base)");
    app.sleep(800);
    app.captureText("states.txt");
    app.stopRecording();
}
