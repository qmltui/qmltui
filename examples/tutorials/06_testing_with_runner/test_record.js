/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Settings ready");
    terminal.marker("Settings");
    terminal.sleep(1000);
    terminal.press("Space");
    terminal.expectText("Logging enabled");
    terminal.marker("Logging enabled");
    terminal.sleep(1000);
}
