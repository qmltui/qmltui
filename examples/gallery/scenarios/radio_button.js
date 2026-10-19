/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Profile: fast");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Space");
    terminal.expectText("Profile: balanced");
    terminal.sleep(800);
    terminal.press("Tab");
    terminal.sleep(500);
    terminal.press("Space");
    terminal.expectText("Profile: safe");
    terminal.sleep(900);
    terminal.captureText("radio_button.txt");
    terminal.stopRecording();
}
