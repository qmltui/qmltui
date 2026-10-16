/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("No clicks yet");
    terminal.expectText("Inspector");
    terminal.expectText("clicks: 0");
    terminal.expectText("focus: Primary");

    // Activate focused Primary button with Space
    terminal.press("SPACE");
    terminal.waitForText("Clicked 1 time");
    terminal.expectText("clicks: 1");
    terminal.expectText("last: Primary");

    // Click checkable Pin button
    terminal.clickText("☆ Pin");
    terminal.waitForText("Clicked 2 times");
    terminal.expectText("★ Pinned");
    terminal.expectText("pin.checked: true");

    // Click Filled button
    terminal.clickText("Filled");
    terminal.waitForText("Clicked 3 times");
    terminal.expectText("last: Filled");

    // Toggle checkable in Inspector
    terminal.clickText("checkable");
    terminal.waitForText("Mode: Checkable");

    // Click Reset button to clear counter and pin state
    terminal.clickText("Reset");
    terminal.waitForText("No clicks yet");
    terminal.expectText("pin.checked: false");
    terminal.expectText("last: Reset");
}
