/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Braille: eight dots per cell");
    terminal.sleep(900);
    terminal.typeText("m");
    terminal.expectText("HalfBlock: two pixels per cell");
    terminal.sleep(900);
    terminal.typeText("m");
    terminal.expectText("ASCII: brightness as characters");
    terminal.sleep(900);
    terminal.captureText("image.txt");
    terminal.stopRecording();
}
