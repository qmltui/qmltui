/** @param {Runner} app */
function main(app) {
    app.expectText("workers=4  port=8080  timeout=30 s");
    app.sleep(600);
    app.press("UP");
    app.press("UP");
    app.expectText("workers=6  port=8080  timeout=30 s");
    app.sleep(600);
    app.press("TAB");
    app.press("DOWN");
    app.expectText("workers=6  port=7980  timeout=30 s");
    app.sleep(600);
    app.press("TAB");
    app.press("RIGHT");
    app.expectText("workers=6  port=7980  timeout=35 s");
    app.sleep(800);
    app.captureText("spin_box.txt");
    app.stopRecording();
}
