/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Host Terminal Feature Detection & Diagnostics");
    terminal.expectText("Terminal Program:");
    terminal.expectText("Color Resolution:");
    terminal.expectText("Synchronized Output:");
    terminal.expectText("Kitty Keyboard:");
    terminal.expectText("Kitty Graphics:");
    terminal.expectText("Sixel Graphics:");
    terminal.expectText("Default Foreground:");
    terminal.expectText("Default Background:");
    terminal.expectText("ANSI Palette");
    terminal.expectText("black");
    terminal.expectText("red");
    terminal.expectText("blue");
    terminal.expectText("white");
    terminal.captureText("capabilities.txt");
}
