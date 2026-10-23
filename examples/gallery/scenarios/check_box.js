/** @param {Runner} app */
function main(app) {
    app.expectText("logging=true  verbose=false  colors=true");
    app.sleep(800);
    app.press("Tab");
    app.sleep(500);
    app.press("Space");
    app.expectText("logging=true  verbose=true  colors=true");
    app.sleep(800);
    app.press("Tab");
    app.sleep(500);
    app.press("Space");
    app.expectText("logging=true  verbose=true  colors=false");
    app.sleep(900);
    app.captureText("check_box.txt");
    app.stopRecording();
}
