/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("TextInput · Single-Line Workbench");
    terminal.expectText("Hello, stranger!");
    terminal.expectText("cursorPos: 0");
    terminal.expectText("length:    0/24");
    terminal.expectText("cursorShape: Block");
    terminal.marker("Initial empty state");
    terminal.sleep(400);

    // Type "quinn" into the focused primary TextInput.
    terminal.typeText("quinn");
    terminal.waitForText("Hello, quinn!");
    terminal.expectText("cursorPos: 5");
    terminal.expectText("length:    5/24");
    terminal.expectText("canUndo:   true");
    terminal.marker("Typed username");
    terminal.sleep(400);

    // Cycle cursorShape via the Inspector button and toggle token masking.
    terminal.clickText("Cycle Cursor");
    terminal.waitForText("cursorShape: Beam");
    terminal.sleep(300);

    terminal.clickText("Mask token");
    terminal.waitForText("echoMode:    Normal");
    terminal.expectText("displayText: sk-live-9042");
    terminal.sleep(300);
}
