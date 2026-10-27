/** @param {Runner} app */
function main(app) {
    app.expectText("Last: nothing yet");
    app.sleep(700);
    const pad = app.itemRect("mousePad");
    app.hover(pad.x + 9, pad.y + 2);
    app.expectText("hover (9, 2)");
    app.expectText("┼ (9,2)");
    app.sleep(600);
    app.click(pad.x + 9, pad.y + 2);
    app.expectText("left click at (9, 2)");
    app.sleep(800);
    app.click(pad.x + 17, pad.y + 4, { button: "right" });
    app.expectText("right click at (17, 4)");
    app.hover(pad.x + 5, pad.y + 1);
    app.expectText("hover (5, 1)");
    app.expectText("×");
    app.sleep(900);
    app.captureText("mouse_area.txt");
    app.stopRecording();
}
