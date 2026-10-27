/** @param {Runner} app */
function main(app) {
    app.expectText("(nothing copied yet)");
    app.sleep(600);
    app.press("SPACE");
    app.expectText("Last payload: ssh deploy@prod.example.com");
    app.expectText("1 copy");
    app.sleep(700);
    app.clickText("Copy Build Cmd");
    app.expectText("Last payload: cmake --build build -j8");
    app.expectText("2 copies");
    app.sleep(800);
    app.captureText("clipboard.txt");
    app.stopRecording();
}
