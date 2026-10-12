/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("STATUS: ASYNC READY");
    terminal.rootObject().requestClipboard();
    terminal.waitForText("CLIPBOARD: Unavailable");
    terminal.clickText("type text here");
    terminal.paste("normal paste\nworks");
    terminal.expectText("INPUT: normal paste works");
    terminal.expectText("CLIPBOARD: Unavailable");
    terminal.captureText("clipboard.txt");
    terminal.press("a", ["Ctrl"]);
    terminal.press("c", ["Ctrl"]);
    terminal.expectText("STATUS: COPY REQUESTED: normal paste works");
    terminal.expectText("INPUT: normal paste works");
    terminal.captureText("copy-feedback.txt");
}
