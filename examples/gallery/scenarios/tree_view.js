/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Nothing activated yet");
    terminal.sleep(700);
    terminal.press("Down");
    terminal.press("Down");
    terminal.sleep(500);
    terminal.press("Enter");
    terminal.expectText("Activated: src/Core/Item.cpp");
    terminal.sleep(700);
    terminal.clickText("Controls");
    terminal.press("Right");
    terminal.expectText("Button.qml");
    terminal.sleep(900);
    terminal.captureText("tree_view.txt");
    terminal.stopRecording();
}
