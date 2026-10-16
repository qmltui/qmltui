/** @param {Terminal} terminal */
function main(terminal) {
    terminal.expectText("ToolTip Stage");
    terminal.expectText("Deploy release v2.4 to prod");
    terminal.expectText("ToolTip.delay: 150ms");
    terminal.sleep(300);

    terminal.press("Tab");
    terminal.expectText("Rollback cluster to v2.3");
    terminal.sleep(300);

    terminal.press("Tab");
    terminal.expectText("Inspect pod health metrics");
    terminal.sleep(300);

    terminal.press("d");
    terminal.expectText("ToolTip.delay: 450ms");
    terminal.sleep(300);
}
