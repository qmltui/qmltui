/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Hello, QmlTui!");
    terminal.expectText("Interactions: 0");
    terminal.sleep(900);

    terminal.press("Enter");
    terminal.expectText("Interactions: 1");
    terminal.sleep(500);

    terminal.press("Enter");
    terminal.expectText("Interactions: 2");
    terminal.sleep(500);

    terminal.press("Enter");
    terminal.expectText("Interactions: 3");
    terminal.sleep(700);

    terminal.press("r");
    terminal.expectText("Interactions: 0");
    terminal.sleep(900);

    terminal.stopRecording();
}
