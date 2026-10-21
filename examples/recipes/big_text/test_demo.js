/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("BigText — 3x5 Block Glyph Item");
    terminal.sleep(3000);
    terminal.stopRecording();
}
