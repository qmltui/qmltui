/** @param {Runner} app */
function main(app) {
    app.expectText("Inline mode");
    app.expectScrollbackText("QmlTui");
    app.expectScrollbackText("Inline Agent CLI");
    app.sleep(700);
    app.typeText("deploy production --canary");
    app.sleep(600);
    app.press("Enter");
    app.expectText("printed: 1");
    app.expectScrollbackText("deploy production --canary");
    app.sleep(900);
    app.captureScrollback("inline_mode_scrollback.txt");
    app.captureText("inline_mode.txt");
    app.stopRecording();
}
