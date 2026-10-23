// Options objects, scoped regions, and objectName item locators.
function main(app) {
    app.waitForText("STATUS: ASYNC READY");

    // Options objects and positional forms are interchangeable.
    app.press("k", { modifiers: [] });
    app.waitForText("KEY K", { timeout: 3000 });
    app.expectText("KEY K", { timeout: 5000 });

    // A scoped region asserts only inside its rectangle.
    const dup = app.region(0, 11, 80, 1);
    dup.expectText("DUPLICATE");
    dup.expectNoText("TESTRUNNER");
    if (dup.findText("DUPLICATE").length !== 1)
        app.fail("region.findText() must clip matches to the region");
    if (app.findText("DUPLICATE").length !== 2)
        app.fail("findText() must still see both matches");
    if (!dup.text().includes("DUPLICATE"))
        app.fail("region.text() must return the region contents");
    dup.waitForText("DUPLICATE");

    // The item locator resolves objectName to a rectangle in cells.
    const status = app.itemRect("statusText");
    if (status.x !== 1 || status.y !== 3)
        app.fail("itemRect() returned the wrong origin: " + status.x + "," + status.y);
    const statusItem = app.item("statusText");
    statusItem.expectText("STATUS:");
    statusItem.waitForText("KEY K");
    app.waitForItem("statusText");

    // A half-off-screen item is clipped to the viewport.
    const clipped = app.itemRect("clippedMarker");
    if (clipped.x !== 78 || clipped.width !== 2)
        app.fail("itemRect() must clip to the viewport, got x=" + clipped.x
                      + " width=" + clipped.width);

    const title = app.regionText(1, 1, 18, 1);
    if (title.trim() !== "TESTRUNNER FIXTURE")
        app.fail("regionText() returned: " + title);
    app.finish();
}
