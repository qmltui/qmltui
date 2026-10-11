/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Content Area 1");
    terminal.advanceTime(200);
    terminal.captureText("panel.txt");
}
