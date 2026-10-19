/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Easing.OutBounce");
    terminal.expectText("Easing.InOutQuad");
    terminal.sleep(500);
    terminal.sleep(500);
    terminal.sleep(500);
    terminal.sleep(500);
    terminal.captureText("animation.txt");
    terminal.stopRecording();
}
