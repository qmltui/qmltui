/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Terminal Capabilities");
    terminal.expectText("Host Terminal Feature Detection & Diagnostics");
    terminal.expectText("Environment & Geometry");
    terminal.expectText("Protocols & Features");
    terminal.expectText("Synchronized Output");
    terminal.expectText("Kitty Keyboard");
    terminal.expectText("Kitty Graphics");
    terminal.expectText("Sixel Graphics");
    terminal.expectText("Color Resolution");
    terminal.expectText("Reports Printed");

    // Click or trigger the print button
    terminal.clickText("Print Report (P)");
    terminal.expectText("Reports Printed:");

    terminal.captureText("capabilities.txt");
}
