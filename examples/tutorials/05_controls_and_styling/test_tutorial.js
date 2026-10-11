/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Logging disabled");
    terminal.press("Space");
    terminal.expectText("Logging enabled");
    terminal.waitForText("40%");
    terminal.clickText("Apply settings");
    terminal.expectText("Save these settings now?");
    terminal.press("Escape");
    terminal.expectText("Settings unchanged");
    terminal.captureText("tutorial.txt");
}
