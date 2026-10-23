/** @param {Runner} app */
function main(app) {
    app.waitForText("STATUS: ASYNC READY");
    app.rootObject().requestClipboard();
    app.waitForText("CLIPBOARD: Unavailable");
    app.clickText("type text here");
    app.paste("normal paste\nworks");
    app.expectText("INPUT: normal paste works");
    app.expectText("CLIPBOARD: Unavailable");
    app.captureText("clipboard.txt");
    app.press("a", ["Ctrl"]);
    app.press("c", ["Ctrl"]);
    app.expectText("STATUS: COPY REQUESTED: normal paste works");
    app.expectText("INPUT: normal paste works");
    app.captureText("copy-feedback.txt");
}
