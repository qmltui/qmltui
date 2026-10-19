/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Last: nothing yet");
    terminal.sleep(700);
    terminal.hover(15, 6);
    terminal.sleep(600);
    terminal.click(15, 6);
    terminal.expectText("left click at");
    terminal.sleep(800);
    terminal.click(20, 7, { button: "right" });
    terminal.expectText("right click at");
    terminal.sleep(900);
    terminal.captureText("mouse_area.txt");
    terminal.stopRecording();
}
