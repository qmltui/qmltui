/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Multi-Color Overlapping Braille Canvas Demo (Qamui)");
    terminal.advanceTime(200);
    terminal.captureText("canvas.txt");
}
