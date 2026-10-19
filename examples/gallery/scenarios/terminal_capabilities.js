/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Host Terminal Feature Detection & Diagnostics");
    terminal.expectText("Terminal Program:");
    terminal.expectText("Color Resolution:");
    terminal.expectText("Synchronized Output:");
    terminal.expectText("Kitty Keyboard:");
    terminal.expectText("Kitty Graphics:");
    terminal.expectText("Sixel Graphics:");
    terminal.expectText("Reports Printed: 1");

    terminal.clickText("Print Report (P)");
    terminal.waitForText("Reports Printed: 2");
    terminal.captureText("capabilities.txt");
}
