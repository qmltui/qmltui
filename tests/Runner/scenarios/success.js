function main(app) {
    app.waitForText("READY");
    app.waitForText("ASYNC READY");
    app.press("K");
    app.waitForText("KEY K");
    app.typeText("2");
    app.waitForText("KEY 2");
    app.typeText("+");
    app.waitForText("KEY PLUS");
    app.press("Tab");
    app.typeText("café 世界");
    app.press("a", ["Shift"]);
    app.waitForText("INPUT: café 世界A");
    app.expectNoText("type text here");
    app.captureText("success.txt");
    app.finish();
}
