/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("BigText — 3x5 Block Glyph Item");
    terminal.advanceTime(200);
    terminal.captureText("big_text.txt");
}
