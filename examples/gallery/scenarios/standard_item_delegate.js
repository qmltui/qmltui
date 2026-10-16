/** @param {Terminal} terminal */
function main(terminal) {
    terminal.waitForText("Release Deployment Checklist");
    terminal.expectText("row: 0");
    terminal.expectText("checked: 2 / 5");
    terminal.expectText("Selected: Verify TLS certificates");

    // Move down to row 2 (Warm edge CDN caches)
    terminal.press("DOWN");
    terminal.waitForText("row: 1");
    terminal.press("DOWN");
    terminal.waitForText("row: 2");
    terminal.expectText("Selected: Warm edge CDN caches");

    // Press Space to toggle row 2 checked
    terminal.press("SPACE");
    terminal.waitForText("checked: 3 / 5");
    terminal.expectText("Last toggled: Warm edge CDN caches");

    // Click Next Row in Inspector -> row 3 (Drain canary traffic)
    terminal.clickText("Next Row");
    terminal.waitForText("row: 3");
    terminal.expectText("Selected: Drain canary traffic");

    // Click Toggle Row in Inspector -> checked: 4 / 5
    terminal.clickText("Toggle Row");
    terminal.waitForText("checked: 4 / 5");
    terminal.expectText("Last toggled: Drain canary traffic");
}
