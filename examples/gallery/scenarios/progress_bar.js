/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Multi-Style Progress Rack (58%)");
    terminal.expectText("value: 58%");
    terminal.expectText("position: 0.58");

    // Press RIGHT to increase shared progressValue by 5 -> 63%
    terminal.press("RIGHT");
    terminal.waitForText("Multi-Style Progress Rack (63%)");
    terminal.expectText("value: 63%");

    // Click +10 button in Inspector -> 73%
    terminal.clickText("+10");
    terminal.waitForText("Multi-Style Progress Rack (73%)");
    terminal.expectText("value: 73%");

    // Click -10 button in Inspector -> 63%
    terminal.clickText("-10");
    terminal.waitForText("Multi-Style Progress Rack (63%)");

    // Toggle showBrackets and showPercentage in Inspector
    terminal.clickText("showBrackets");
    terminal.waitForText("brackets: false");

    terminal.clickText("showPercentage");
    terminal.waitForText("percent: false");
}
