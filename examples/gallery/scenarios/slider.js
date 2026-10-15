/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Volume  42%");
    terminal.sleep(700);
    for (let i = 0; i < 8; ++i)
        terminal.press("Right");
    terminal.expectText("Volume  50%");
    terminal.sleep(700);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Right");
    terminal.expectText("Contrast  4");
    terminal.sleep(900);
    terminal.captureText("slider.txt");
    terminal.stopRecording();
}
