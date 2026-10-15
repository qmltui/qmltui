/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Default caption");
    terminal.expectText("Centred caption");
    terminal.expectText("Right caption");
    terminal.sleep(1200);
    terminal.captureText("panel.txt");
    terminal.stopRecording();
}
