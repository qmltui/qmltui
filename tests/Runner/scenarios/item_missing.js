// A hidden item is not locatable: it contributes no cells to the frame.
function main(app) {
    app.waitForText("TESTRUNNER FIXTURE");
    app.itemRect("hiddenMarker");
    app.fail("itemRect() must fail for an invisible item");
}
