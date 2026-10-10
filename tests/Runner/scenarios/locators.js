// Options objects, scoped regions, and objectName item locators.
function main(terminal) {
    terminal.waitForText("STATUS: ASYNC READY");

    // Options objects and positional forms are interchangeable.
    terminal.press("k", { modifiers: [] });
    terminal.waitForText("KEY K", { timeout: 3000 });
    terminal.expectText("KEY K", { timeout: 5000 });

    // A scoped region asserts only inside its rectangle.
    const dup = terminal.region(0, 11, 80, 1);
    dup.expectText("DUPLICATE");
    dup.expectNoText("TESTRUNNER");
    if (dup.findText("DUPLICATE").length !== 1)
        terminal.fail("region.findText() must clip matches to the region");
    if (terminal.findText("DUPLICATE").length !== 2)
        terminal.fail("findText() must still see both matches");
    if (!dup.text().includes("DUPLICATE"))
        terminal.fail("region.text() must return the region contents");
    dup.waitForText("DUPLICATE");

    // The item locator resolves objectName to a rectangle in cells.
    const status = terminal.itemRect("statusText");
    if (status.x !== 1 || status.y !== 3)
        terminal.fail("itemRect() returned the wrong origin: " + status.x + "," + status.y);
    const statusItem = terminal.item("statusText");
    statusItem.expectText("STATUS:");
    statusItem.waitForText("KEY K");
    terminal.waitForItem("statusText");

    // A half-off-screen item is clipped to the viewport.
    const clipped = terminal.itemRect("clippedMarker");
    if (clipped.x !== 78 || clipped.width !== 2)
        terminal.fail("itemRect() must clip to the viewport, got x=" + clipped.x
                      + " width=" + clipped.width);

    const title = terminal.regionText(1, 1, 18, 1);
    if (title.trim() !== "TESTRUNNER FIXTURE")
        terminal.fail("regionText() returned: " + title);
    terminal.finish();
}
