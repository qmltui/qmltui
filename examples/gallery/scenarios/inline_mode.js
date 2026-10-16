/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Inline mode");
    terminal.expectText("printed: 0");
    terminal.marker("Initial inline prompt");
    terminal.sleep(400);

    // Submit first command into Scrollback.
    terminal.typeText("hello from inline");
    terminal.press("Enter");
    terminal.waitForText("printed: 1");
    terminal.expectScrollbackText("hello from inline");
    terminal.expectScrollbackText("committed #1");
    terminal.marker("First command committed");
    terminal.sleep(400);

    // Submit second command into Scrollback.
    terminal.typeText("deploy --prod");
    terminal.press("Enter");
    terminal.waitForText("printed: 2");
    terminal.expectScrollbackText("deploy --prod");
    terminal.expectScrollbackText("committed #2");
    terminal.sleep(300);
}
