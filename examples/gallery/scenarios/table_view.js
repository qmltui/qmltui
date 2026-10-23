/** @param {Runner} app */
function main(app) {
    app.expectText("Select a row or click a header");
    app.expectText("qmltui_gallery");
    app.sleep(700);
    app.press("Down");
    app.sleep(400);
    app.press("Down");
    app.sleep(500);
    app.press("Enter");
    app.expectText("Activated row 1: qmltui_gallery");
    app.sleep(800);
    app.clickText("CPU%");
    app.expectText("Header 2 clicked");
    app.sleep(900);
    app.captureText("table_view.txt");
    app.stopRecording();
}
