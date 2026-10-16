/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("Modal Dialog Stage");
    terminal.expectText("No decision yet");
    terminal.expectText("Delete report.pdf…");
    terminal.sleep(300);

    terminal.press("Enter");
    terminal.expectText("report.pdf will be removed");
    terminal.sleep(400);

    terminal.press("Escape");
    terminal.expectText("Kept report.pdf");
    terminal.expectText("Decisions: 1");
    terminal.sleep(300);

    terminal.press("Enter");
    terminal.expectText("report.pdf will be removed");
    terminal.sleep(400);

    terminal.press("Enter");
    terminal.expectText("Deleted report.pdf");
    terminal.expectText("Decisions: 2");
    terminal.sleep(300);
}
