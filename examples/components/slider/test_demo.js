/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("QmlTui Slider Component Example");
    terminal.sleep(1500);
    terminal.press("Tab");
    terminal.sleep(600);
    terminal.press("Right");
    terminal.sleep(400);
    terminal.press("Right");
    terminal.sleep(400);
    terminal.press("Right");
    terminal.sleep(400);
    terminal.press("Right");
    terminal.sleep(400);
    terminal.press("Right");
    terminal.sleep(1000);
    terminal.press("Tab");
    terminal.sleep(600);
    terminal.press("Right");
    terminal.sleep(400);
    terminal.press("Right");
    terminal.sleep(400);
    terminal.press("Right");
    terminal.sleep(1500);
    terminal.stopRecording();
}
