/** @param {Runner} app */
function main(app) {
    app.expectText("Nothing activated yet");
    app.sleep(700);
    app.press("Down");
    app.press("Down");
    app.sleep(500);
    app.press("Enter");
    app.expectText("Activated: src/Core/Item.cpp");
    app.sleep(700);
    app.clickText("Controls");
    app.press("Right");
    app.expectText("Button.qml");
    app.sleep(900);
    app.captureText("tree_view.txt");
    app.stopRecording();
}
