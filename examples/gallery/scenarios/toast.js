/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Toast Dispatcher");
    terminal.expectText("Last Toast: None");
    terminal.sleep(300);

    terminal.press("Enter");
    terminal.expectText("Last Toast: [info] Syncing");
    terminal.expectText("Total Sent: 1");
    terminal.sleep(300);

    terminal.clickText("Success");
    terminal.expectText("Last Toast: [success] Saved");
    terminal.expectText("Total Sent: 2");
    terminal.sleep(300);

    terminal.press("p");
    terminal.expectText("bottom-right");
    terminal.clickText("Warning");
    terminal.expectText("Last Toast: [warning] Quota");
    terminal.expectText("Total Sent: 3");
    terminal.sleep(400);
}
