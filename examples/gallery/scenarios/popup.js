/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Popup Overlay Stage");
    terminal.expectText("Open Command Popup");
    terminal.expectText("Last Command: None");
    terminal.sleep(300);

    terminal.press("Enter");
    terminal.expectText("Command Palette");
    terminal.expectText("Select a quick action:");
    terminal.sleep(400);

    terminal.press("Enter");
    terminal.expectText("Last Command: Sync Nodes");
    terminal.expectText("Opened: 1");
    terminal.expectText("Closed: 1");
    terminal.sleep(300);

    terminal.press("Enter");
    terminal.expectText("Command Palette");
    terminal.sleep(300);

    terminal.press("Escape");
    terminal.expectText("Closed: 2");
    terminal.sleep(300);
}
