/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui.Controls ProgressBar Showcase");
    terminal.advanceTime(200);
    terminal.captureText("progress_bar.txt");
}
