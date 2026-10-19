/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Terminal Capabilities");
    terminal.expectText("Host Terminal Feature Detection & Diagnostics");
    terminal.expectText("Environment & Geometry");
    terminal.expectText("Protocols & Features");
    terminal.expectText("Synchronized Output");
    terminal.expectText("Kitty Keyboard");
    terminal.expectText("Kitty Graphics");
    terminal.expectText("Sixel Graphics");
    terminal.expectText("Color Resolution");
    terminal.expectText("Reports Printed:");

    terminal.clickText("Print Report (P)");
    terminal.waitForText("2");
    terminal.captureText("capabilities.txt");
}
