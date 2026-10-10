// A hidden item is not locatable: it contributes no cells to the frame.
function main(terminal) {
    terminal.waitForText("TESTRUNNER FIXTURE");
    terminal.itemRect("hiddenMarker");
    terminal.fail("itemRect() must fail for an invisible item");
}
