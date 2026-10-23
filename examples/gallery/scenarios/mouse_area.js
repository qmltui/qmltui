/** @param {Runner} app */
function main(app) {
    app.expectText("Last: nothing yet");
    app.sleep(700);
    app.hover(15, 6);
    app.sleep(600);
    app.click(15, 6);
    app.expectText("left click at");
    app.sleep(800);
    app.click(20, 7, { button: "right" });
    app.expectText("right click at");
    app.sleep(900);
    app.captureText("mouse_area.txt");
    app.stopRecording();
}
