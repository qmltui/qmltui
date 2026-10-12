/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui.Controls ProgressBar Showcase");
    terminal.advanceTime(200);
    terminal.captureText("progress_bar.txt");
}
