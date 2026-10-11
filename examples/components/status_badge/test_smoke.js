/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Qamui StatusBadge Component Demo");
    terminal.advanceTime(200);
    terminal.captureText("status_badge.txt");
}
