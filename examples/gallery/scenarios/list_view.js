/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Current: Ada Lovelace");
    terminal.sleep(800);
    terminal.press("Down");
    terminal.expectText("Current: Grace Hopper");
    terminal.sleep(700);
    terminal.press("Down");
    terminal.expectText("Current: Linus Torvalds");
    terminal.sleep(700);
    terminal.press("Down");
    terminal.expectText("Current: Margaret Hamilton");
    terminal.sleep(900);
    terminal.captureText("list_view.txt");
    terminal.stopRecording();
}
