/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Shortcut");
    terminal.expectText("Inspector");
    terminal.expectText("enabled: true");
    terminal.expectText("activations: 0");
    terminal.sleep(450);

    terminal.press("2");
    terminal.expectText("2: Test");
    terminal.expectText("activations: 1");
    terminal.sleep(450);

    terminal.press("3");
    terminal.expectText("3: Deploy");
    terminal.expectText("activations: 2");
    terminal.sleep(450);

    terminal.press("1");
    terminal.expectText("1: Build");
    terminal.expectText("activations: 3");
    terminal.sleep(450);

    terminal.press("c");
    terminal.expectText("activations: 0");
    terminal.expectText("Cleared");
    terminal.sleep(450);
}
